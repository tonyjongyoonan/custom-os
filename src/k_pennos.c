#include "k_pennos.h"
#include "signals.h"
#include "errors.h"
#include <time.h>
#include <string.h>
#define MAX_OPEN_FILES 128
#include "scheduler.h"

// maybe have to include global variables for init process?

Deque* global_pcbs;
pid_t current_pid;
PCB* current_pcb;

// init pcb with id 1
PCB* init_process; // shell process is same as init process
pid_t init_pid;
int pid_counter;

extern clock_t system_start_time;
extern FILE* logFile;
extern Deque* zombie_pcbs;
extern Deque* stopped_pcbs;

extern int current_quantum;

void k_system_init() {
    global_pcbs = Deque_Allocate();
    global_pcbs -> num_elements = 0;
    global_pcbs -> front = NULL;
    global_pcbs -> back = NULL;
    // parent = -1 because init/shell doesn't have a parent
    int empty_fds[MAX_OPEN_FILES];
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        empty_fds[i] = -1;
    }
    empty_fds[0] = 0;
    empty_fds[1] = 1;
    init_process = init_pcb(1, -1, empty_fds, 2, -1);
    init_process->process_name = "init";
    init_pid = 1;
    pid_counter = 2;
    Deque_Push_Back(global_pcbs, init_process);
    current_pcb = init_process;
    current_pid = init_pid;
}

PCB* get_pcb_from_pid(pid_t pid) {
    DequeNode* current = global_pcbs -> front;
    while (current != NULL) {
        if (current -> pcb -> pid == pid) {
            return current -> pcb;
        }
        current = current -> next;
    }
    
    // pid doesn't exist
    printf("No PCB for PID");
    return NULL;
}

void update_children(PCB* parent, pid_t pid) {
    // update children of parent
    parent->num_children++;
    parent->children_pids[parent->num_children-1] = pid;
    if (parent->num_children == 100) {
        parent->children_pids = realloc(parent->children_pids, (parent->num_children + 100) * sizeof(pid_t));
    }
}


// KERNEL CALLS
// create a new child thread and associated PCB.
// The new thread should retain much of the properties of the parent.
// The function should return a reference to the new PCB
PCB* k_process_create(PCB* parent, char* name) {
    pid_t pid = pid_counter;
    pid_counter++;
    pid_t parent_pid = parent -> pid;
    // child inherits open fds of its parent
    int parent_open_fds[MAX_OPEN_FILES];
    memcpy(parent_open_fds, parent->open_fds, sizeof(parent_open_fds[0]) * MAX_OPEN_FILES);
    int parent_num_open_files = parent -> num_open_fds;
    
    int priority = parent -> priority;
    if (parent->pid == 2) {
        priority = 0;
    }

    PCB* new_pcb = init_pcb(pid, parent_pid, parent_open_fds, parent_num_open_files, priority);
    new_pcb->process_name = name;
    // add new_pcb to global_pcbs
    Deque_Push_Back(global_pcbs, new_pcb);
    
    update_children(parent, pid);

    // logging
    double time = ((double)(clock() - system_start_time));
    fprintf(logFile, "[%d] CREATE\t\t\t%d\t%d\t%s\n", current_quantum, new_pcb->pid, new_pcb->priority, new_pcb->process_name);
    fflush(logFile);
    return new_pcb;
}


char* signal_to_string(pennos_signal signal) {
    switch (signal) {
        case S_SIGSTOP:
            return "S_SIGSTOP";
            break;
        case S_SIGTERM:
            return "S_SIGTERM";
            break;
        case S_SIGCONT:
            return "S_SIGCONT";
            break;
        default:
            return "no signal";
            break;
    }
    return "";
}

// kill the process referenced by process with the signal
void k_process_kill(PCB* process, pennos_signal signal) {
    // check if process is already terminated
    if (process -> status == TERMINATED) {
        printf("Kill sent to Finished Process");
        return;
    }
    
    switch (signal) {
        case S_SIGSTOP:
            // stop the process
            process -> status = STOPPED;
            process->e_status = EXIT_STOPPED;
            if (strcmp(process->process_name, "sleep") == 0) {
                schedule_sleep_process(process, S_SIGSTOP);
            }
            waitpid_checks(process);
            break;
        case S_SIGTERM:
            // terminate the process
            process -> status = ZOMBIE;
            process->e_status = EXIT_SIGNAL;
            if (strcmp(process->process_name, "sleep") == 0) {
                schedule_sleep_process(process, S_SIGTERM);
            }
            waitpid_checks(process);
            break;
        case S_SIGCONT:
            // continue the process
            process -> status = READY;
            if (strcmp(process->process_name, "sleep") == 0) {
                schedule_sleep_process(process, S_SIGCONT);
            } 
            fprintf(logFile, "[%d] CONTINUED\t\t\t%d\t%d\t%s\n", current_quantum, process->pid, process->priority, process->process_name);
            fflush(logFile);
            break;
        default:
            printf("Unhandled signal\n");
    }

    // logging
    fprintf(logFile, "[%d] SIGNALED\t\t\t%d\t%d\t%s\n", current_quantum, process->pid, process->priority, process->process_name);
    fflush(logFile);
}

void remove_pid(pid_t* arr, int *size, pid_t pid) {
    for (int i = 0; i < *size; ++i) {
        if (arr[i] == pid) {
            // Shift the elements to fill the gap
            for (int j = i; j < *size - 1; ++j) {
                arr[j] = arr[j + 1];
            }
            // Decrease the size of the array
            (*size)--;
            break;
        }
    }
}

// called when a terminated/finished thread’s resources needs to be cleaned up. 
// Such clean-up may include freeing memory, setting the status of the child, etc.
//orphans
void k_process_cleanup(PCB *process) {
    // uhh just do this for now, prob dont actually need the terminated check
    if (process -> status != TERMINATED) {
        process->status = TERMINATED;

        for (int i = 0; i < process -> num_children; i++) {
            // set the orphan's parent to init
            pid_t child_pid = process -> children_pids[i];
            // printf("cleanup child pid from %d: %d\n", child_pid, process->pid);
            PCB* child_pcb = get_pcb_from_pid(child_pid);
            child_pcb -> parent_pid = 1; // new parent is init
            fprintf(logFile, "[%d] ORPHAN\t\t\t%d\t%d\t%s\n", current_quantum, child_pid, child_pcb->priority, child_pcb->process_name);
            fflush(logFile);
        }
        // cleanup its parent
        PCB* parent_pcb = get_pcb_from_pid(process->parent_pid);
        remove_pid(parent_pcb->children_pids, &parent_pcb->num_children, process->pid);
        fprintf(logFile, "[%d] WAITED\t\t\t%d\t%d\t%s\n", current_quantum, process->pid, process->priority, process->process_name);
        fflush(logFile);
        // free_pcb(process);
    } else {
        // printf("error: trying to cleanup already terminated process %d, %d\n", process->pid, process->status);
        printf("Attempted to cleanup already terminated process\n");
    }
}

void k_logout() {
    exit(EXIT_SUCCESS);
}

static char* status_to_char(process_status st) {
    switch(st) {
        case READY: return "R";
        case BLOCKED: return "B";
        case STOPPED: return "S";
        case RUNNING: return "R";
        case ZOMBIE: return "Z";
        case TERMINATED: return "T";
    }
}

//Display pid, ppid, and priority.
void k_print() {
    printf("PID\tPPID\tPRI\tSTAT\tCMD\n");
    DequeNode* node = global_pcbs->front->next;
    while (node != NULL) {
        PCB* pcb = node->pcb;
        if (pcb->status != TERMINATED) {
            printf("%d\t%d\t%d\t%s\t%s\n", pcb->pid, pcb->parent_pid, pcb->priority, status_to_char(pcb->status), pcb->process_name);
        }
        node = node->next;
    }
}

//print all jobs
void k_jobs() {
     DequeNode* first = global_pcbs->front;
    while (first != NULL) {
        PCB* pcb = first->pcb;
        if (pcb->status != TERMINATED) {
            printf("%d: %s\n", pcb->pid, pcb->process_name);
        }
        first = first->next;
    }
}

void k_background_wait(int case_value, int background_id, char* waited_pid_name) {
    if (case_value == -1) { //DONE
        printf("[%d]\tDone\t[%s]\n", background_id, waited_pid_name);
    } else if (case_value == 0) { //Stopped
        printf("[%d]\tStopped\t[%s]\n", background_id, waited_pid_name);
    } 
}

void k_background_status(int current_background_id, int pid) {
    printf("[%d]\t%d\n", current_background_id, pid);
}