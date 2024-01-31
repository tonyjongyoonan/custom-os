/**
 * @file k_pennos.h
 * @brief Header file defining the kernel-level functions of PennOS.
 *
 */
#ifndef K_PENNOS_H
#define K_PENNOS_H
#define _XOPEN_SOURCE 500
#include <signal.h> 	// sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> 	// dprintf, fputs, perror
#include <stdlib.h> 	// malloc, free
#include <sys/time.h> 	// setitimer
#include <ucontext.h> 	// getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> 	// read, usleep, write
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include "pcb.h"
#include "Deque.h"
#include "signals.h"
#include "errors.h"

/**
 * @brief Initializes the kernel system.
 * This function initializes the init process and the global_pcb deques, which keeps track of all the 
 * processes that have been spawned.
 */
void k_system_init(void);

/**
 * @brief Creates a process.
 * This function creates a PCB for the new process, updates the parent that spawned this process, and adds
 * the process to the global_pcb deque.
 * @param parent The PCB of the parent spawning the new process.
 * @param name The name of the process being created.
 * @return a pointer to the PCB of the newly created process.
 */
PCB* k_process_create(PCB* parent, char* name);

/**
 * @brief Sends a signal to the specified process
 * This function sets the status of the targeted process to the appropriate status based on the signal being sent
 * and checks for any processes waiting on the targeted process.
 * @param process The PCB of the process that the signal will be sent to.
 * @param signal The signal to send.
 */
void k_process_kill(PCB* process, pennos_signal signal);

/**
 * @brief Cleans up the PCB when a process is terminated.
 * This function sets the status of the input process to terminated, frees any associated memory with the process,
 * handles the orphanized children of the terminated process, and updates the parent's information about its children.
 * @param process The PCB of the process to be cleaned-up.
 */
void k_process_cleanup(PCB *process);

/**
 * @brief A helper function to get the PCB value from a pid value.
 * @param pid The pid of the process.
 * @return A pointer to the PCB of the process.
 */
PCB* get_pcb_from_pid(pid_t pid);

/**
 * @brief Logs the user out.
 *
 * The user gets logged out of the current running process.
 */
void k_logout();


/**
 * @brief Display process ID (pid), parent process ID (ppid), and priority.
 *
 * This function prints the process ID (pid), parent process ID (ppid), 
 * and priority of the current process.
 */
void k_print();


/**
 * @brief Prints all current jobs running on PennOS.
 *
 * This function iterates through all the jobs on PennOS and prints information about each job.
 *
 * @return void
 */
void k_jobs(void);

/**
 * @brief Waits for a background process to complete.
 *
 * This function waits for the completion of a background process identified by its background ID.
 *
 * @param case_value An integer specifying the case for waiting.
 * @param background_id The ID of the background process to wait for.
 * @param waited_pid_name A pointer to a character array to store the name of the waited PID.
 */
void k_background_wait(int case_value, int background_id, char* waited_pid_name);

/**
 * @brief Prints the status of a background process.
 *
 * This function retrieves the status of a background process based on its background ID and waited PID.
 *
 * @param current_background_id The ID of the current background process.
 * @param pid The PID of the process being waited for.
 */
void k_background_status(int current_background_id, int pid);

#endif /* K_PENNOS_H */