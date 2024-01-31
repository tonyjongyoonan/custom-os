/**
 * @file scheduler.h
 * @brief Header file defining the kernel-level scheduler functions and scheduler-related helper functions for other kernel-level
 * PennOS functions.
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H
#define _XOPEN_SOURCE 500
#include <signal.h> 	// sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> 	// dprintf, fputs, perror
#include <stdlib.h> 	// malloc, free
#include <sys/time.h> 	// setitimer
#include <ucontext.h> 	// getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> 	// read, usleep, write
#include <sys/types.h>
#include <stdbool.h>
#include <valgrind/valgrind.h>
#include "pcb.h"
#include "signals.h"

/**
 * @brief Sets up the stack for a ucontext.
 * This function allocates memory for a ucontext stack and registers it with valgrind.
 * @param stack Pointer to the stack_t structure to be initialized.
 */
static void setStack(stack_t *stack);

/**
 * @brief Sets up a Ready process to be scheduled by adding it to one of the scheduler's active priority queue's.
 * @param pcb The PCB of the process to be scheduled.
 */
void schedule_ready_process(PCB* pcb);

/**
 * @brief Sets the a process' priority
 * @param pcb The PCB of the process whose priority will be changed
 * @param priority The new priority of the process.
 * **/
void set_priority(PCB* pcb, int priority);

/**
 * @brief Runs the scheduler
 * This function runs in a continual loop. A new process will be chosen to be run each quantum according to the priority ratios
 * established for PennOS. The chosen context will be swapped to and allowed to run for one quantum before switching back to the
 * scheduler. The process is then checked for any status changes and is handled accordingly (either by rescheduling into an active
 * priority queue, or addition into one of the zombie, stopped, blocked queues). At the end of each scheduler loop, there are checks
 * for status changes in the zombie, stopped, and blocked queues which are then handled accordingly.
 */
void scheduler_main(void);

/**
 * @brief Initializes the scheduler
 * This function initializes the negative (-1), zero (0), positive (1) queues for active processes, as well as the stopped, blocked,
 * and zombie queues. This function also initializes the idle PCB, which will be scheduled when there are zero active processes in
 * any priority queues.
 */
void init_scheduler(void);

/**
 * @brief Gets the pid of the last stopped process. This is done by accessing the stopped_pcbs deque.
 * @return The pid of the last stopped process.
 * **/
pid_t get_last_stopped_pcb(void);

/**
 * @brief Handles a change in the status of a sleep process.
 * The function handles a change to a stopped, running/blocked, and finished/zombie'd state for a sleep process.
 * @param pcb The PCB of the sleep process.
 * @param sig The signal describing the status change.
 * **/
void schedule_sleep_process(PCB* pcb, pennos_signal sig);

/**
 * @brief Checks for any process waiting on the input process. Usually called upon a process' status change. Also handles any
 * resulting state changes for the waiting and waited on processes.
 * 
 * @param pcb The PCB of the process that could be waited on.
 * @return A true boolean if the input process has been waited on.
 * **/
bool waitpid_checks(PCB* pcb);

#endif /* SCHEDULER_H */