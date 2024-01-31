#include "p_pennos.h"
#include "k_pennos.h"
#include <valgrind/valgrind.h>
#include <time.h>
#include "signals.h"
#include "errors.h"
#include <string.h>
#include "Deque.h"
#include "scheduler.h"
#include "f_pennos.h"


// error macros
#define ERRNO errno
#define MAX_OPEN_FILES 128

// system init upon booting shell?
extern Deque* global_pcbs;
extern PCB* current_pcb;
extern FILE* logFile;

extern clock_t system_start_time;

extern int current_quantum;

// called when the thread finished
// thread should just wait until the alarm is sounded
static void exit_context() {
    // change so that actual status change is made by kernel?
    current_pcb->status = ZOMBIE;
    current_pcb->e_status = EXIT_NORMAL; // TODO: this line is done in p_exit() too, remove one of them

    // logging statement for normal exit
    double time = (double)(clock() - system_start_time);
    fprintf(logFile, "[%d] EXITED\t\t\t%d\t%d\t%s\n", current_quantum, current_pcb->pid, current_pcb->priority, current_pcb->process_name);
    fflush(logFile);
    // wait for end of quantum
    while(1);
}

static ucontext_t* make_exitcontext() {
    ucontext_t* uc = malloc(sizeof(ucontext_t));
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);
    setStack(&uc->uc_stack);

    makecontext(uc, exit_context, 0);
    return uc;
}

// move to a separate file later on
static void setStack(stack_t *stack) {
    void *sp = malloc(SIGSTKSZ);
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ);
    *stack = (stack_t) { .ss_sp = sp, .ss_size = SIGSTKSZ };
}

static int get_num_args(char* argv[]) {
    int i = 0;
    while (argv[i] != NULL) {
        i++;
    }
    return i;
}

// may have to handle arg having different possible types (int or string??)
static void make_context(ucontext_t *ucp, void (*func)(), char *argv[]) {

    int num_args = get_num_args(argv);
    
    getcontext(ucp);
    sigemptyset(&ucp->uc_sigmask);
    setStack(&ucp->uc_stack);
    ucp->uc_link = make_exitcontext();

    switch(num_args) {
        case 0:
            makecontext(ucp, func, 0);
            break;
        case 1:
            if (atoi(argv[0]) != 0) {
                makecontext(ucp, func, 1, atoi(argv[0]));
            } else {
                makecontext(ucp, func, 1, argv[0]); // calls func with argument thread
            }
            break;
        case 2:
            makecontext(ucp, func, 2, argv[0], argv[1]);
            break;
        case 3:
            makecontext(ucp, func, 3, argv[0], atoi(argv[1]), atoi(argv[2]));
            break;
        default:
            p_perror("Too many arguments", ArgumentNotFoundError);
    }
    
}

void p_system_init(void (*shell_func)()) {
    k_system_init();
    init_scheduler();
    char* argv[] = { NULL };
    p_spawn(shell_func, argv, STDIN_FILENO, STDOUT_FILENO, "shell");
}

void make_context_cmd(ucontext_t *ucp, void (*func)(), struct parsed_command *cmd) {
    getcontext(ucp);
    sigemptyset(&ucp->uc_sigmask);
    setStack(&ucp->uc_stack);
    ucp->uc_link = make_exitcontext();
    
    makecontext(ucp, func, 1, cmd);
}

static char* status_to_string(process_status s) {
    switch(s) {
        case READY:
            return "READY";
        case BLOCKED:
            return "BLOCKED";
        case STOPPED:
            return "STOPPED";
        case RUNNING:
            return "RUNNING";
        case ZOMBIE:
            return "ZOMBIE";
        case TERMINATED:
            return "TERMINATED";
        default:
            printf("invalid status");
            return NULL;
    }
}

// USER CALLS
// forks a new thread that retains most of the attributes of the parent thread
// Once the thread is spawned, it executes the function referenced by func with its argument array argv. 
// fd0 is the file descriptor for the input file, and fd1 is the file descriptor for the output file. 
// It returns the pid of the child thread on success, or -1 on error.
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1, char* name) {

    PCB* new_pcb = k_process_create(current_pcb, name);

    // put fd0 and fd1, which are pcb_fds, into locations of pcb's 0 and 1
    int pcb_in_global_fd = new_pcb->open_fds[fd0];
    int pcb_out_global_fd = new_pcb->open_fds[fd1];

    new_pcb->open_fds[0] = pcb_in_global_fd;
    new_pcb->open_fds[1] = pcb_out_global_fd;

    // create ucontext to execute func
    make_context(new_pcb -> uc, func, argv);

    schedule_ready_process(new_pcb);
    // printf("new pcb with priority: %d\n", new_pcb->priority);
    return new_pcb -> pid;
}

pid_t p_spawn_cmd(void (*func)(), struct parsed_command *cmd, int fd0, int fd1, char* name) { //fd0 and fd1 are global fds
    PCB* new_pcb = k_process_create(current_pcb, name);

    // put fd0 and fd1, which are pcb_fds, into locations of pcb's 0 and 1
    int pcb_in_global_fd = new_pcb->open_fds[fd0];
    int pcb_out_global_fd = new_pcb->open_fds[fd1];

    new_pcb->open_fds[0] = pcb_in_global_fd;
    new_pcb->open_fds[1] = pcb_out_global_fd;

    // create ucontext to execute func
    make_context_cmd(new_pcb -> uc, func, cmd);

    schedule_ready_process(new_pcb);
    return new_pcb -> pid;
}


// sets the calling thread as blocked (if nohang is false) until a child of the calling thread changes state. 
// It is similar to Linux waitpid(2). If nohang is true, p_waitpid does not block but returns immediately. 
// p_waitpid returns the pid of the child which has changed state on success, or -1 on error


pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang) {
    // get pcb of pid
    DequeNode* current = global_pcbs -> front;
    if (current_pcb->num_children == 0) {
        return -1;
    }

    // when pid = -1, wait on change
    if (pid == -1) {
        PCB* caller_pcb = current_pcb;
        if (nohang) {
            for (int i = 0; i < caller_pcb->num_children; i++) {
                pid_t child_pid = caller_pcb->children_pids[i];
                int status;
                p_waitpid(child_pid, &status, nohang);
                if (W_WIFEXITED(status) || W_WIFSIGNALED(status) || W_WIFSTOPPED(status)) {
                    return child_pid;
                }
            }
            return 0;
        } else {
            current_pcb->status = BLOCKED;
            current_pcb->waitpid_pid = -1;

            while(current_pcb->status == BLOCKED);

            // current_pcb->waitpid_pid will be changed to whatever's been waiting on if its -1
            pid_t waited_pid = current_pcb->waitpid_pid;
            current_pcb->waitpid_pid = 0;
            PCB* waited_pcb = get_pcb_from_pid(waited_pid);
            k_process_cleanup(waited_pcb);
            return waited_pid;
        }
    }

    PCB* target_pcb = get_pcb_from_pid(pid);
    if (target_pcb->parent_pid != current_pcb->pid) {
        p_perror("Wrong Child waited on", WrongProcessWaitedError);
        return -1;
    }
    

    if (target_pcb == NULL) {
        p_perror("Nonexistent PID", ProcessNotFoundError);
        return -1;
    } else if (target_pcb->status == TERMINATED) {
        // process is terminated
        p_perror("Already waited on", ProcessWaitError);
        return -1;
    }
    
    if (nohang) {
        exit_status target_status;
        switch (target_pcb->status) {
            case READY:
            case RUNNING:
            case BLOCKED:
                target_status = NOT_EXITED;
                break;                
            case STOPPED:
                target_status = target_pcb->e_status;
                break;
            case ZOMBIE:
                target_status = target_pcb->e_status;
                k_process_cleanup(target_pcb);
                break;
            default:
                p_perror("Unexpected status in nohang", StatusNotFoundError);
                return -1;
        }
        *wstatus = target_status;
    } else {
        current_pcb->status = BLOCKED;
        current_pcb->waitpid_pid = pid;

        // wait to end of quantum to switch back, process will put in blocked queue by scheduler.
        while(current_pcb->status == BLOCKED);
        *wstatus = current_pcb->waitpid_estatus;
        current_pcb->waitpid_pid = 0;
        if (current_pcb->waitpid_estatus == EXIT_NORMAL || current_pcb->waitpid_estatus == EXIT_SIGNAL) {
            k_process_cleanup(target_pcb);
        }
        current_pcb->waitpid_estatus = NO_CHANGE;
    }
    
    return pid;
}


// sends the signal sig to the thread referenced by pid. It returns 0 on success, or -1 on error
int p_kill(pid_t pid, pennos_signal sig) {
    // iterate over global_pcbs to get pcb block
    PCB* kill_target = get_pcb_from_pid(pid);
    if (!kill_target) {
        p_perror("Nonexistent PCB for PID", PCBNotFoundError);
        return -1;
    }
    k_process_kill(kill_target, sig);
    return 0;
}

// exits the current thread unconditionally
// should switch to a finished_context
void p_exit() {
    // wait for next clock tick for scheduler to take back control - do this in uc_link
    current_pcb->status = ZOMBIE;
    current_pcb->e_status = EXIT_NORMAL;
    for(int i = 2; i < MAX_OPEN_FILES; i++) {
        if (current_pcb->open_fds[i] >= 2 && current_pcb->open_fds[i] < MAX_OPEN_FILES) {
            f_close(i);
        }
    }
    while(1);
}

// sets the priority of the thread pid to priority.
// TODO: what is the int return value?
int p_nice(pid_t pid, int priority) {
    PCB* pcb = get_pcb_from_pid(pid);
    if (pcb == NULL) {
        return -1;
    }
    //log change in niceness
    double time = (double)(clock() - system_start_time);
    fprintf(logFile, "[%d] NICE\t\t\t%d\t%d\t%s\n", current_quantum, pcb->pid, pcb->priority, pcb->process_name);
    fflush(logFile);
    set_priority(pcb, priority);
    return 0;
}

// sets the calling process to blocked until ticks of the system clock elapse, and then sets the thread to running. 
// Importantly, p_sleep should not return until the thread resumes running; 
// however, it can be interrupted by a S_SIGTERM signal. 
// Like sleep(3) in Linux, the clock keeps ticking even when p_sleep is interrupted.
pid_t p_sleep(unsigned int ticks) {
    // set to blocked
    PCB* new_pcb = k_process_create(current_pcb, "sleep");
    new_pcb->sleep_counter = ticks;
    // current_pcb->sleep_fin_time = clock() + ticks;
    new_pcb->status = BLOCKED; //set to blocked
    schedule_ready_process(new_pcb);

    return new_pcb->pid;

    // //logging
    //enque to blocked, log that's blocked, call to swap back to scheduler context
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
void p_print() {
    k_print();
}

//print all jobs
void p_jobs() {
    k_jobs();
}

// bring the last stopped or backgrounded job to the foreground, or 
// the job specified by job_id.
// returns true if successful, false otherwise
pid_t p_fg(pid_t pid) {
    if (pid == -1) {
        // change this to get_last_fg_pcb
        pid = get_last_stopped_pcb();
    }

    if (pid == -1) {
        p_perror("No stopped processes", ProcessNotFoundError);
        return -1;
    }
    
    PCB* pcb = get_pcb_from_pid(pid);

    if (pcb->sleep_counter > 0) {
        if (pcb->status != BLOCKED) {
            schedule_sleep_process(pcb, S_SIGCONT);
        }
    } else {
        pcb->status = READY;
        schedule_ready_process(pcb);
    }
    return pid;
}

// continue the last stopped job, or the job specified by job_id. 
// Note that this does mean you will need to implement the & operator in your shell.
pid_t p_bg(pid_t pid) {
    if (pid == -1) {
        pid = get_last_stopped_pcb();
    }

    if (pid == -1) {
        p_perror("No stopped processes", ProcessNotFoundError);
        return -1;
    }

    PCB* pcb = get_pcb_from_pid(pid);

    if (pcb->sleep_counter > 0) {
        if (pcb->status != BLOCKED) {
            schedule_sleep_process(pcb, S_SIGCONT);
        }
    } else {
        pcb->status = READY;
        schedule_ready_process(pcb);
    }
    return pid;
}

// error printing function
void p_perror(char *message, pennos_error err) {
    f_fprintf(stderr, "%s: ", message);

    // Print the error description 
    f_fprintf(stderr, "%s\n", mapEnumToString(err));
}


char* get_pcb_name_from_pid(pid_t pid){
    DequeNode* current = global_pcbs -> front;
    while (current != NULL) {
        if (current -> pcb -> pid == pid) {
            return current -> pcb->process_name;
        }
        current = current -> next;
    }
    
    // pid doesn't exist
    p_perror("No PCB Name found", PCBNotFoundError);
    return NULL;
}

// returns true if the child terminated normally, that is, by a call to p_exit or by returning
bool W_WIFEXITED(int status) {
    return status == EXIT_NORMAL;
}

bool W_WIFSTOPPED(int status) {
    return status == EXIT_STOPPED;
}

bool W_WIFSIGNALED(int status) {
    return status == EXIT_SIGNAL;
}

void start_os() {
    scheduler_main();
}

void p_logout() {
    k_logout();
}

void p_background_wait(int case_value, int background_id, char* waited_pid_name) {
    k_background_wait(case_value, background_id, waited_pid_name);
}

void p_background_status(int current_background_id, int pid) {
    k_background_status(current_background_id, pid);
}