#include "Deque.h"
#include "scheduler.h"
#include "k_pennos.h"

#include <time.h>

#define THREAD_COUNT 19

static ucontext_t mainContext;
static ucontext_t schedulerContext;
static ucontext_t threadContexts[THREAD_COUNT];
static ucontext_t *activeContext = threadContexts;
static const int centisecond = 10000; // 10 milliseconds

static const int quantum = 10 * centisecond;


int curr_thread = 0;
static int priority_tracker[19];
static int tracker_pos;

Deque* neg_priority;
Deque* zero_priority;
Deque* pos_priority;

// after every quantum, check if any blocked/stopped pcbs have become unblocked/continued
// want to maintain the guarantee that every pcb in a queue is valid (i.e. every pcb in blocked_pcb is blocked, etc.)
Deque* blocked_pcbs;
Deque* stopped_pcbs;
Deque* zombie_pcbs;

// queue containing stopped and backgrounded jobs
Deque* fg_pcb_queue;

extern pid_t current_pid;
extern PCB* current_pcb;
extern Deque* global_pcbs;
extern FILE* logFile;

extern PCB* init_process;
int global_int = 0;

clock_t system_start_time;

bool is_sleeping = false;
int num_ready_processes;

int current_quantum = 0;

PCB* idle_pcb;

void idle() {
    sigset_t mask, old_mask;
    while (1) {
        sigfillset(&mask);
        sigdelset(&mask, SIGALRM);
        sigprocmask(SIG_SETMASK, &mask, &old_mask);
        sigsuspend(&old_mask);
    }
}
// move to a separate file later on
static void setStack(stack_t *stack) {
    void *sp = malloc(SIGSTKSZ * 100);
    VALGRIND_STACK_REGISTER(sp, sp + SIGSTKSZ * 100);
    *stack = (stack_t) { .ss_sp = sp, .ss_size = SIGSTKSZ * 100};
}

PCB* create_idle_pcb() {
    int empty_fds[MAX_OPEN_FILES];
    memset(empty_fds, -1, sizeof(empty_fds));
    empty_fds[0] = 0;
    empty_fds[1] = 1;
    idle_pcb = init_pcb(0, 0, empty_fds, 0, -1);
    ucontext_t* uc = idle_pcb->uc;
    getcontext(uc);
    sigemptyset(&uc->uc_sigmask);
    setStack(&uc->uc_stack);
    makecontext(uc, idle, 0);
    return idle_pcb;
}

Deque* get_pq_from_priority(int priority) {
    switch(priority) {
        case -1:
            return neg_priority;
        case 0:
            return zero_priority;
        case 1:
            return pos_priority;
        default:
            printf("error: undefined priority");
            return NULL;
    }
}

static void alarmHandler(int signum) {  // SIGALARM
    swapcontext(current_pcb->uc, &schedulerContext);
}

static void setAlarmHandler(void) {
    struct sigaction act;
    act.sa_handler = alarmHandler;
    act.sa_flags = SA_RESTART;
    sigfillset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
}

static void setTimer(void) {
  struct itimerval it;
  it.it_interval = (struct timeval) { .tv_usec = centisecond * 10 };
  it.it_value = it.it_interval;
  setitimer(ITIMER_REAL, &it, NULL);
}


// initialization for deques
void init_scheduler(void) {
    neg_priority = Deque_Allocate();

    zero_priority = Deque_Allocate();

    pos_priority = Deque_Allocate();

    blocked_pcbs = Deque_Allocate();

    stopped_pcbs = Deque_Allocate();

    zombie_pcbs = Deque_Allocate();

    Deque_Push_Back(blocked_pcbs, init_process);
    PCB* idle_pcb = create_idle_pcb();
    idle_pcb->process_name = "idle";
    // printf("initiatlized\n");
    system_start_time = clock();
}

//schedules a process - change state to ready / add it back into a queue
void schedule_ready_process(PCB* pcb) {
    // printf("called schedule ready process on %d, %s\n", pcb->pid, pcb->process_name);
    if (pcb->status == READY) {
        Deque* pq = get_pq_from_priority(pcb->priority);
        // printf("deque: %d\n", pcb->priority);
        // printf("deque size: %d\n", pq->num_elements);
        Deque_Push_Back(pq, pcb);

        double time = (double)(clock() - system_start_time);
    } else if (pcb->sleep_counter > 0) { //sleeping process
    // } else if (pcb->sleep_fin_time > 0) {
        Deque_Push_Back(blocked_pcbs, pcb);

        // is_sleeping = true;
    } else {
        printf("pcb not ready: pid %d\n", pcb->pid);
        return;
    }
    
}

//sets the priority of a process
void set_priority(PCB* pcb, int priority) {
    int old_priority = pcb->priority;
    pcb -> priority = priority;
    if (pcb -> status == READY) {
        // remove it from its current deque and add to new one
        Deque* old_priority_dq = get_pq_from_priority(old_priority);
        Deque_Pop_PID(old_priority_dq, pcb->pid);

        schedule_ready_process(pcb);
    }
}

void set_bg(PCB* pcb) {
    DequeNode* stopped = stopped_pcbs->front;
    while (stopped) {
        stopped->pcb->is_background = true; //bring to background
        stopped = stopped -> next;
    }
}



static void create_array(void) {
    for (int i = 0; i < 19; i++) {
        if (i == 0 || i == 3 || i == 6 || i == 9) {
            priority_tracker[i] = 1;
        } else if (i == 1 || i == 4 || i == 7 || i == 10 || i == 12 || i == 14) {
            priority_tracker[i] = 0;
        } else {
            priority_tracker[i] = -1;
        }
    }
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

// called by scheduler_main() when a process changes state
// returns true if a process is waiting for this process.
// we are assuming that process can only be waited on by parent, i.e. at most one process is waiting on it
bool waitpid_checks(PCB* pcb) {
    DequeNode* blocked_pcb = blocked_pcbs->front;
    // printf("entered waitpid_checks!!!!\n");

    while (blocked_pcb) {
        // printf("in blocked pcb: %d, %s\n", blocked_pcb->pcb->pid, blocked_pcb->pcb->process_name);
        // check if a blocked process is the parent of this process
        if (blocked_pcb->pcb->pid == pcb->parent_pid) {
            // if blocked parent is waiting on any process, let it wait on this one
            if (blocked_pcb->pcb->waitpid_pid == -1) {
                blocked_pcb->pcb->waitpid_pid = pcb->pid;
            }
            // check that blocked parent is waiting on this process
            if (blocked_pcb->pcb->waitpid_pid == pcb->pid) {
                // printf("pcb status: %s\n", status_to_string(pcb->status));
                // unblock the waiting process (if it is not being waited on by init)
                if (blocked_pcb->pcb->pid != 1 && pcb->status != READY) {
                    blocked_pcb->pcb->status = READY;
                    if (pcb->status == ZOMBIE) {
                        // printf("REACHES HERE\n");
                        blocked_pcb->pcb->waitpid_estatus = pcb->e_status;
                    } else if (pcb->status == READY) {
                        // blocked_pcb->pcb->waitpid_estatus = CONTINUED;
                        blocked_pcb->pcb->waitpid_estatus = NOT_EXITED;
                    } else if (pcb->status == STOPPED) {
                        blocked_pcb->pcb->waitpid_estatus = EXIT_STOPPED;
                        pcb->e_status = NO_CHANGE;
                    }

                    fprintf(logFile, "[%d] UNBLOCKED\t\t\t%d\t%d\t%s\n", current_quantum, blocked_pcb->pcb->pid, blocked_pcb->pcb->priority, blocked_pcb->pcb->process_name);
                    // fprintf(stderr, "[%d] UNBLOCKED        %d\t%d\t%s\n", current_quantum, blocked_pcb->pcb->pid, blocked_pcb->pcb->priority, blocked_pcb->pcb->process_name);
                    fflush(logFile);
                    schedule_ready_process(blocked_pcb->pcb);
                } else if (blocked_pcb->pcb->pid == 1) {
                    // init waiting
                    printf("waited on by init\n");
                } else {
                    printf("reaady asldkfjslakdjf\n");
                }
                
                return true;
            }
            break;
        }
        blocked_pcb = blocked_pcb->next;
    }
    return false;
}

pid_t get_last_stopped_pcb() {
    DequeNode* last_stopped = stopped_pcbs->back;
    while (last_stopped) {
        if (last_stopped->pcb->status==STOPPED) {
            // last_stopped->pcb->status = READY;
            // schedule_ready_process(last_stopped->pcb);
            return last_stopped->pcb->pid;
        }
        last_stopped = last_stopped->prev;
    }
    // no stopped threads, return false
    return -1;
}

void schedule_sleep_process(PCB* pcb, pennos_signal sig) {
    switch (sig) {
        case S_SIGSTOP: 
            // add to stopped queue
            pcb->status = STOPPED;
            Deque_Pop_PID(blocked_pcbs, pcb->pid);
            Deque_Push_Back(stopped_pcbs, pcb);
            waitpid_checks(pcb);
            break;
        case S_SIGTERM:
            // zombied, then waited on
            pcb->status = ZOMBIE;
            Deque_Pop_PID(blocked_pcbs, pcb->pid);
            Deque_Push_Back(zombie_pcbs, pcb);
            waitpid_checks(pcb);
            break;
        case S_SIGCONT:
            // continue the process
            pcb->status = BLOCKED;
            Deque_Pop_PID(stopped_pcbs, pcb->pid);
            Deque_Push_Back(blocked_pcbs, pcb);
            waitpid_checks(pcb);
            break;
        default:
            printf("unhandled signal: %d", sig);
            return;
    }

}


// rewrite of scheduler to fix some compile errors
void scheduler_main() {
    create_array();
    setAlarmHandler();
    // question: quantum timer includes the time to schedule?
    setTimer();

    tracker_pos = 0;

    while(1) {
        global_int += 1;

        if (tracker_pos >= 19) {
            tracker_pos = 0;
        }
        
        // TODO: check that pcb -> status = READY and pcb -> priority hasn't changed after popping
        Deque* pq = get_pq_from_priority(priority_tracker[tracker_pos]);
        // printf("pq size: %d\n", pq->num_elements);
        // check if queue is empty or not
        if (neg_priority->num_elements == 0 && pos_priority->num_elements == 0 && zero_priority->num_elements == 0) {
            Deque_Push_Back(pq, idle_pcb);
        }

        if (!Deque_Pop_Front(pq, &current_pcb)) {
            //CPU check
            // sigsuspend()
            tracker_pos++;
            continue;
        }

        if (current_pcb -> status != READY) {
            // handle incorrect status
            if (current_pcb->status == ZOMBIE) {
                Deque_Push_Back(zombie_pcbs, current_pcb);
            } else if (current_pcb->status == STOPPED) {
                Deque_Push_Back(stopped_pcbs, current_pcb);
            } else {
                // printf("skipping process: %d\n", current_pcb->pid);
            }
            tracker_pos++;
            continue;
        } else if (current_pcb -> priority != priority_tracker[tracker_pos]) {
            // handle incorrect priority
            tracker_pos++;
            continue;
        }
        current_quantum += 1;
        current_pid = current_pcb -> pid;

        fprintf(logFile, "[%d] SCHEDULE\t\t\t%d\t%d\t%s\n", current_quantum, current_pid, current_pcb->priority,current_pcb->process_name);
        fflush(logFile);

        // save current context in schedulerContext, start executing uc:
        // https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-swapcontext-save-restore-user-context, https://linux.die.net/man/3/swapcontext
        current_pcb->status = RUNNING;
        swapcontext(&schedulerContext, current_pcb->uc);
        // if (current_pcb->priority == 0) {
        //     printf("iter : %s\n", current_pcb->process_name);
        // }
        
        // returned from quantum
        if (current_pcb->status == RUNNING) {
            // process is still running. schedule it again.
            current_pcb->status = READY;
            schedule_ready_process(current_pcb);
        } else if (current_pcb->status == ZOMBIE || current_pcb->status == STOPPED) {
            // process has changed state (either signaled, exited normally, or stopped)
            // if process has is zombie, check if any processes are wa
            // if process was stopped by a signal, check if process was being waitpid'd by blocked process
            // check if any processes were waiting on this process
            // fprintf(stderr, "ZOMBIE %s", current_pcb->process_name);
            // printf("entered waitpid from 1 (zombie check): %d, %s\n", current_pcb->pid, status_to_string(current_pcb->status));
            bool is_waited_on = waitpid_checks(current_pcb);
            
            if (current_pcb->status == TERMINATED) {
                k_process_cleanup(current_pcb);
            } else if (current_pcb->status == ZOMBIE) {
                fprintf(logFile, "[%d] ZOMBIE\t\t\t%d\t%d\t%s\n", current_quantum, current_pcb->pid, current_pcb->priority, current_pcb->process_name);
                fflush(logFile);
                Deque_Push_Back(zombie_pcbs, current_pcb);
            } else if (current_pcb->status == STOPPED) {
                fprintf(logFile, "[%d] STOPPED\t\t\t%d\t%d\t%s\n", current_quantum, current_pcb->pid, current_pcb->priority, current_pcb->process_name);
                // fprintf(stderr, "[%d] STOPPED       %d\t%d\t%s\n", (int)time, current_pcb->pid, current_pcb->priority, current_pcb->process_name);
                fflush(logFile);
                Deque_Push_Back(stopped_pcbs, current_pcb);
            } else {
                printf("unexpected status: %s", status_to_string(current_pcb->status));
            }
            
        } else if (current_pcb->status == BLOCKED) {
            // current pcb is blocked (e.g. by calling waitpid, sleep)
            fprintf(logFile, "[%d] BLOCKED\t\t\t%d\t%d\t%s\n", current_quantum, current_pcb->pid, current_pcb->priority, current_pcb->process_name);
            fflush(logFile);
            Deque_Push_Back(blocked_pcbs, current_pcb);
        } else {
            printf("current pcb: %s\n", current_pcb->process_name);
            printf("unexpected status in scheduler main: %s\n", status_to_string(current_pcb->status));
        }

        // sleep checks
        DequeNode* sleep_node = blocked_pcbs->front;
        clock_t curr_time = clock();
        while (sleep_node) {
            if (sleep_node->pcb->sleep_counter > 0) {
                // printf("sleep: %d, %s\n", sleep_node->pcb->sleep_counter, sleep_node->pcb->process_name);

            // if (sleep_node->pcb->sleep_fin_time > 0) {
                PCB* sleep_pcb = sleep_node->pcb;
                sleep_pcb->sleep_counter -= quantum;

                if (sleep_pcb->sleep_counter <= 0) {
                    // sleep has finished
                    sleep_pcb->status = ZOMBIE;
                    // sleep_pcb->sleep_fin_time = -1;
                    sleep_pcb->sleep_counter = -1;
                    sleep_pcb->e_status = EXIT_NORMAL;
                    // printf("waiting\n");
                    // fprintf(stderr, "[%d] EXITED\t\t\t%d\t%d\t%s\n", current_quantum, sleep_pcb->pid, sleep_pcb->priority, sleep_pcb->process_name);

                    fprintf(logFile, "[%d] EXITED\t\t\t%d\t%d\t%s\n", current_quantum, sleep_pcb->pid, sleep_pcb->priority, sleep_pcb->process_name);
                    fflush(logFile);
                    waitpid_checks(sleep_pcb);
                }
            }
            sleep_node = sleep_node->next;
        }
        
        // need to check for any resumed stopped processes
        DequeNode* stop_pcb = stopped_pcbs->front;
        while (stop_pcb) {
            // printf("in stop pcb: %d\n", stop_pcb->pcb->pid);
            if (stop_pcb->pcb->status == READY) {
                // remove from this queue, add to ready
                PCB* resumed_pcb = stop_pcb->pcb;
                Deque_Pop_Pcb(stopped_pcbs, stop_pcb);
                // printf("entered waitpid_checks from stop\n");
                waitpid_checks(resumed_pcb);
                // printf("what\n");
                resumed_pcb->status = READY;
                schedule_ready_process(resumed_pcb);
                // printf("started pid: %d %d\n", resumed_pcb->pid, stopped_pcbs->num_elements);
            }
            stop_pcb = stop_pcb->next;
        }
        tracker_pos++;      

        DequeNode* zombie = zombie_pcbs->front;
        while (zombie) {
            // printf("pid: %d %s\n", zombie->pcb->pid, zombie->pcb->process_name);
            if (zombie->pcb->status != TERMINATED) {
                bool waited = waitpid_checks(zombie->pcb);
            }
            zombie = zombie->next;
            // if (waited) {
            //     Deque_Pop_Pcb(zombie_pcbs, zombie);
            // }
        }

    }

}
