/**
 * @file bash.h
 * @brief Header file defining simple shell function for PennOS.
 *
 * This file contains function declarations for various commands and operations
 * that the PennOS shell can perform.
 */
#ifndef BASH_H
#define BASH_H
#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include "signals.h"
#include "f_pennos.h"

// uhhh check these function definitions over
// for now, cat without a file name (just repeats what is typed in)
/**
 * @brief Simulate the 'sleep' command in the shell.
 *
 * @param n Number of seconds to sleep.
 */
void bash_sleep(int n);

/**
 * @brief Simulate a busy process consuming CPU cycles.
 */
void busy(void);

/**
 * @brief Simulate the 'echo' command in the shell.
 *
 * @param cmd Parsed command.
 */
void bash_echo(struct parsed_command *cmd);

/**
 * @brief Sends signals like S_SIGTERM, S_SIGSTOP, and S_SIGCONT to a process.
 *
 * @param sig Signal type (S_SIGCONT, S_SIGTERM, S_SIGSTOP).
 * @param pid Process ID.
 */
void bash_kill(char* signal, char* pid_str);

/**
 * @brief Spawns a zombie process.
 * Spawns a process that loops infinitely. That process spawns a process that exits immediately.
 */
void zombify(void);

/**
 * @brief Spawns an orphan process.
 * Spawns a process that spawns a child which loops infinitely. The parent process then exits.
 */
void orphanify(void);

/**
 * @brief Prints process ids, process names, process statuses, and process priorities for non-terminated processes.
 */
void bash_ps(void);

/**
 * @brief Simulate the 'nice' command in the shell which changes a process' priority level.
 *
 * @param priority Priority level.
 * @param str Name of the command to be executed.
 * @param argv Arguments for the command.
 */
void bash_nice(int priority, char* str, char* argv);

/**
 * @brief Set the priority of a process by PID.
 *
 * @param priority Priority level.
 * @param pid Process ID.
 */
void nice_pid(int priority, int pid);

/**
 * @brief Simulate the 'jobs' command in the shell.
 * Prints out the currently running processes.
 */
void jobs(void);

/**
 * @brief Starts a background job.
 * If a job id isn't specified, then it brings the last stopped job to the foreground.
 *
 * @param job_id Process ID.
 */
void bg(pid_t job_id);

/**
 * @brief Bring a background job to the foreground.
 * If a job id isn't specified, then it brings the last backgrounded or stopped job to the foreground.
 *
 * @param job_id Process ID.
 * @param status Status of resulting mount
 */
void fg(pid_t job_id);

// file system calls

/**
 * @brief Mounts the provided file system
 * If a file system is not valid, then it prints an error message.
 *
 * @param fs_name Name of the file system.
 * @param status Status of resulting mount (-1 if unsuccessful, 0 if successful)
 */
void bash_mount(const char *fs_name, int *status);


/**
 * @brief Creates an empty file if it does not exist,
 * or update its timestamp otherwise.
 *
 * @param cmd Parsed command.
 *
 */
void bash_touch(struct parsed_command *cmd);

/**
 * @brief Removes the specified file if it exists.
 *
 * @param fs_name Name of the file system.
 */
void bash_rm(const char *fs_name);

/**
 * @brief Renames the specified file if it exists.
 *
 * @param src Name of the file to be renamed.
 * @param dst Name of the file to be renamed to.
 */
void bash_mv(const char *src, const char *dst);

/**
 * @brief Copies specified file to destination if it exists.
 *
 * @param cmd Parsed command which contains information about how to copy the file.
 */
void bash_cp(struct parsed_command *cmd);

/**
 * @brief The usual cat from bash, concatenates as specified.
 * 
 * @param cmd Parsed command which contains information about how to concatenate the file.
 */
void bash_cat(struct parsed_command *cmd);

/**
 * @brief Lists all files in the current directory.
 */
void bash_ls();

/**
 *  @brief Changes permissions of specified file
 *
 * @param mode Permissions to change to
 * @param fs_name Name of the file to change permissions of
 */
void bash_chmod(const char* mode, const char* fs_name);

/**
 * @brief A secret easter egg we created! 
 */
void egg();

#endif