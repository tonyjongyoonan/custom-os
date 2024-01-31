/**
 * @file p_pennos.h
 * @brief Header file for PennOS, the Penn Shell Operating System.
 *
 * This file includes necessary declarations and dependencies for the PennOS system,
 * including signal handling, standard input/output, memory allocation, and other utilities.
 */

#ifndef P_PENNOS_H
#define P_PENNOS_H



#define _XOPEN_SOURCE 500
#define MAX_OPEN_FILES 128
#include <signal.h> 	// sigaction, sigemptyset, sigfillset, signal
#include <stdio.h> 	// dprintf, fputs, perror
#include <stdlib.h> 	// malloc, free
#include <sys/time.h> 	// setitimer
#include <ucontext.h> 	// getcontext, makecontext, setcontext, swapcontext
#include <unistd.h> 	// read, usleep, write
#include <sys/types.h>
#include <stdbool.h>
#include "signals.h"
#include "parser.h"
#include "errors.h"


/**
 * @brief Initializes the system with the provided shell function process.
 *
 * This function initializes the system with the given shell function process.
 *
 * @param shell_func A pointer to the shell function process to initialize the system with.
 *                  This function should have the signature: `void shell_func()`.
 *
 * @return void
 */
void p_system_init(void (*shell_func)());

/**
 * @brief Spawns a new process.
 *
 * This function forks a new thread that retains most of the attributes of the parent thread.
 * Once the thread is spawned, it executes the function referenced by `func` with its argument array `argv`.
 * `fd0` is the file descriptor for the input file, and `fd1` is the file descriptor for the output file.
 * The function returns the process ID (PID) of the child thread on success, or -1 on error.
 *
 * @param func      A pointer to the function to be executed by the spawned process.
 *                  This function should have the signature: `void func()`.
 * @param argv      An array of strings representing the arguments passed to the spawned process.
 * @param fd0       The input file descriptor for the spawned process.
 * @param fd1       The output file descriptor for the spawned process.
 * @param name      The name of the process.
 *
 * @return pid_t    Returns the process ID (PID) of the spawned process on success, or -1 on error.
 */
pid_t p_spawn(void (*func)(), char *argv[], int fd0, int fd1, char* name);

/**
 * @brief Waits for a process to change state.
 *
 * Depending on the value of the `nohang` parameter, this function either waits (and thus hangs)
 * for a child process of the calling thread to change state or returns immediately if set to true.
 * It is similar to Linux waitpid(2). If `nohang` is true, `p_waitpid` does not block but returns immediately.
 * The calling thread is set as blocked (if `nohang` is false) until a child of the calling thread changes state.
 *
 * @param pid       The process ID (PID) of the process to wait for.
 * @param wstatus   A pointer to an integer where the exit status of the waited process will be stored.
 * @param nohang    If set to true, the function returns immediately without waiting for the process.
 *                  If set to false, the function waits until the specified process exits.
 *
 * @return pid_t    Returns the process ID (PID) of the waited process on success, or -1 on error.
 */
pid_t p_waitpid(pid_t pid, int *wstatus, bool nohang);

/**
 * @brief Sends a signal to a process to terminate it.
 *
 * This function sends the specified signal to the process identified by its process ID (PID).
 *
 * @param pid   The process ID (PID) of the process to be terminated.
 * @param sig   The signal to be sent. See the `pennos_signal` enumeration for valid signal values.
 *
 * @return int  Returns 0 on success, or -1 on error.
 */
int p_kill(pid_t pid, pennos_signal sig);

/**
 * @brief Exits the current thread unconditionally.
 *
 * This function unconditionally exits the current thread and switches to a finished context.
 *
 * @return void
 */
void p_exit(void);

/**
 * @brief Sets the priority of the thread referenced by PID.
 *
 * This function updates the priority of the thread referenced by PID to the specified priority value.
 *
 * @param pid       The process ID (PID) of the thread to update the priority.
 * @param priority  The new priority value to set for the thread.
 *
 * @return int      Returns 0 on success, or -1 on error.
 */
int p_nice(pid_t pid, int priority);

/**
 * @brief Sets the sleep counter of a sleeping thread to the specified number of ticks.
 *
 * This function blocks the calling process until the specified number of system clock ticks elapse,
 * and then sets the sleeping thread to the running state. Importantly, `p_sleep` does not return
 * until the thread resumes running, but it can be interrupted by a `S_SIGTERM` signal. 
 * The clock keeps ticking even if `p_sleep` is interrupted, similar to `sleep(3)` in Linux.
 *
 * @param ticks   The number of clock ticks to set the sleep counter.
 *
 * @return pid_t  Returns the process ID (PID) of the updated sleeping thread.
 */
pid_t p_sleep(unsigned int ticks);


/**
 * @brief Displays the PID, parent PID, and priority of all threads.
 *
 * This function iterates through all the threads and displays each thread's PID, parent PID, and priority.
 *
 * @return void
 */
void p_print(void);

/**
 * @brief Prints all current jobs running on PennOS.
 *
 * This function iterates through all the jobs on PennOS and prints information about each job.
 *
 * @return void
 */
void p_jobs(void);

/**
 * @brief Brings the last stopped or backgrounded job to the foreground or the specified job by PID.
 *
 * This function brings the last stopped or backgrounded job to the foreground or the job specified by `pid`.
 * Returns the process ID (PID) of the updated job if successful; otherwise, returns -1.
 *
 * @param pid   The process ID (PID) of the job to bring to the foreground.
 *
 * @return pid_t  Returns the process ID (PID) of the updated job if successful; otherwise, returns -1.
 */
pid_t p_fg(pid_t pid);


/**
 * @brief Continues the last stopped job or the job specified by PID.
 *
 * This function continues the last stopped job or the job specified by `pid`. 
 * Note that this implies the need to implement the '&' operator in your shell.
 *
 * @param pid   The process ID (PID) of the job to continue.
 *
 * @return pid_t  Returns the process ID (PID) of the updated job if successful; otherwise, returns -1.
 */
pid_t p_bg(pid_t pid);

/**
 * @brief Prints the user-specified error message and the message mapped to the PennOS error.
 *
 * This function prints the user-specified error message (`message`) and the message mapped to the PennOS error (`err`).
 *
 * @param message   The user-specified error message to be printed.
 * @param err       The PennOS error code for which a mapped message will be printed.
 *
 * @return void
 */
void p_perror(char *message, pennos_error err);

/**
 * @brief Gets the process name of the thread associated with the given Process ID (PID).
 *
 * This function retrieves the process name of the thread corresponding to the specified Process ID (PID).
 *
 * @param pid   The Process ID (PID) of the thread.
 * @return char* A pointer to the process name of the thread.
 */
char* get_pcb_name_from_pid(pid_t pid);


/**
 * @brief Returns true if the child terminated normally, either by a call to p_exit or by returning.
 *
 * This function evaluates whether the child process terminated normally. 
 * A return value of true indicates normal termination, while false indicates abnormal termination.
 *
 * @param status The status value obtained from the child process.
 * @return bool  Returns true if the child terminated normally, false otherwise.
 */
bool W_WIFEXITED(int status);

/**
 * @brief Returns true if the child stopped normally, usually by SIGTSTP.
 *
 * This function evaluates whether the child process stopped normally.
 * A return value of true indicates normal stopping, while false indicates abnormal stopping.
 *
 * @param status The status value obtained from the child process.
 * @return bool  Returns true if the child stopped normally; otherwise, returns false.
 */
bool W_WIFSTOPPED(int status);


/**
 * @brief Evaluates whether the child process exited due to a signal.
 *
 * This function determines whether the child process exited because of a signal.
 * A return value of true indicates that the child exited due to a signal, while false
 * indicates that the child process terminated normally.
 *
 * @param status The status value obtained from the child process.
 * @return bool  Returns true if the child process exited due to a signal; otherwise, returns false.
 */
bool W_WIFSIGNALED(int status);


/**
 * @brief Forks a new process that retains most attributes of the parent process.
 *
 * This function forks a new process that retains most attributes of the parent process.
 * Once the process is spawned, it executes the function referenced by `func` with the argument array `argv`
 * inside the `parsed_command` structure pointed to by `cmd`. `fd0` is the file descriptor for the input file,
 * and `fd1` is the file descriptor for the output file.
 *
 * @param func   A pointer to the function to be executed by the spawned process.
 *               This function should have the signature: `void func()`.
 * @param cmd    A pointer to the `parsed_command` structure containing command and argument information.
 * @param fd0    The input file descriptor for the spawned process.
 * @param fd1    The output file descriptor for the spawned process.
 * @param name   The name of the process.
 *
 * @return pid_t  Returns the process ID (PID) of the spawned process.
 */
pid_t p_spawn_cmd(void (*func)(), struct parsed_command *cmd, int fd0, int fd1, char* name);


/**
 * @brief Invokes the scheduler to start the operating system.
 *
 * This function initiates the scheduler, triggering the start of the operating system.
 * It serves as the entry point for launching the OS and coordinating task scheduling.
 *
 * @return void
 */
void start_os();

/**
 * @brief Invokes the scheduler to logout the user.
 *
 * This function stops all processes and logs the user out.
 *
 * @return void
 */
void p_logout();

/**
 * @brief Waits for a background process to complete.
 *
 * This function waits for the completion of a background process identified by its background ID.
 *
 * @param case_value An integer specifying the case for waiting.
 * @param background_id The ID of the background process to wait for.
 * @param waited_pid_name A pointer to a character array to store the name of the waited PID.
 */
void p_background_wait(int case_value, int background_id, char* waited_pid_name);
/**
 * @brief Prints the status of a background process.
 *
 * This function retrieves the status of a background process based on its background ID and waited PID.
 *
 * @param current_background_id The ID of the current background process.
 * @param pid The PID of the process being waited for.
 */
void p_background_status(int current_background_id, int pid);

#endif /* P_PENNOS_H */