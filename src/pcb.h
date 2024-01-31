/**
 * @file pcb.h
 * @brief Header file defining the Process Control Block (PCB) structure and related functions.
 *
 * This file defines the Process Control Block (PCB) structure and functions to initialize and free PCBs.
 */
#ifndef PCB_H
#define PCB_H

#define _XOPEN_SOURCE 500
#define MAX_OPEN_FILES 128
#include <ucontext.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Enumeration of process statuses.
 */
typedef enum {
    READY,
    BLOCKED,
    STOPPED,
    RUNNING,
    ZOMBIE,
    TERMINATED
} process_status;

/**
 * @brief Enumeration of exit statuses. Used for the implementation of waitpid.
 */
typedef enum {
    NOT_EXITED,
    EXIT_NORMAL,
    EXIT_STOPPED,
    EXIT_SIGNAL,
    NO_CHANGE,
} exit_status;

/**
 * @struct pcb_struct
 * @brief Process Control Block (PCB) structure.
 */
typedef struct pcb_struct {
    ucontext_t *uc; // ucontext of thread
    pid_t pid; // thread's process id
    pid_t parent_pid; // parent's process id
    pid_t *children_pids; // pointer to array of children pids
    int num_children; // number of children, for the array
    int open_fds[MAX_OPEN_FILES]; // pointer to array of open fds
    int num_open_fds; // number of open fds for array
    int priority; // priority level of -1, 0, 1
    process_status status;

    char* process_name;
    bool is_background;

    pid_t waitpid_pid; // pid to wait for
    exit_status waitpid_estatus; // exit status of pid to wait for
    exit_status e_status;
    int sleep_counter; //sleep counter
    // clock_t sleep_fin_time;
} PCB;

/**
 * @brief Initialize a Process Control Block (PCB).
 *
 * @param pid Process id.
 * @param parent Parent process id.
 * @param open_fds Array of open file descriptors.
 * @param num_open_fds Number of open file descriptors.
 * @param priority Priority level.
 * @return Initialized PCB.
 */
PCB* init_pcb(pid_t pid, pid_t parent, int *open_fds, int num_open_fds, int priority);

/**
 * @brief Free a Process Control Block (PCB).
 *
 * @param pcb PCB to be freed.
 */
void free_pcb(PCB *pcb);

#endif /* PCB_H */