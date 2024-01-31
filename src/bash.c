#include "bash.h"
#include "p_pennos.h"
#include <string.h>

#define F_WRITE 1
#define F_READ 2
#define F_APPEND 3
#define _XOPEN_SOURCE 500
#define MAX_LINE_LENGTH 4096

#include <time.h>
void bash_sleep(int n) {
    // p_sleep(n * CLOCKS_PER_SEC);
    p_exit();
}

void busy(void) {
    while(1);
    p_exit();
}

void bash_echo(struct parsed_command *cmd) {
    char buffer[MAX_LINE_LENGTH];

    if (cmd->stdin_file != NULL) {
        int bytes_read = f_read(0, MAX_LINE_LENGTH, buffer);
        int bytes_write = f_write(1, buffer, bytes_read);
    } else {
        int i = 1;
        while (cmd->commands[0][i] != NULL) {
            int bytes_write = f_write(1, cmd->commands[0][i], strlen(cmd->commands[0][i]));
            int bytes_write2 = f_write(1, " ", 1);
            i++;
        }
    }
    f_write(1, "\n", 1);
    p_exit();
}

// handles term, stop, cont (S_SIGTERM, S_SIGSTOP, and S_SIGCONT)
// void bash_kill(pennos_signal sig, pid_t pid) {
//     p_kill(pid, sig);
//     p_exit();
// }

void bash_kill(char* signal, char* pid_str) {
    int pid = atoi(pid_str);
    if (strcmp(signal, "-term") == 0) {
        p_kill(pid, S_SIGTERM);
    } else if (strcmp(signal, "-cont") == 0) {
        p_kill(pid, S_SIGCONT);
    } else if (strcmp(signal, "-stop") == 0 ) {
        p_kill(pid, S_SIGSTOP);
    } else {
        // add perror
    }
    p_exit();
}


//list all processes on PennOS. Display pid, ppid, and priority.
void bash_ps() {
    p_print();
    p_exit();
}


//set the priority of the command to priority and execute the command
void bash_nice(int priority, char* str, char* argv) {
    
    p_exit();
}

//adjust the nice level of process pid to priority
void nice_pid(int priority, int pid) {
    
}

//prints all jobs
void jobs() {
    
}

// ZOMBIE
void zombie_child() {
    p_exit();
}

void zombify(void) {
    char* argv[] = {NULL};
    p_spawn(zombie_child, argv, STDERR_FILENO, STDERR_FILENO, "zombie_child");
    while(1);
    p_exit();
}

// ORPHAN
void orphan_child() {
    while(1);
    p_exit();
}

void orphanify(void) {
    char* argv[] = {NULL};
    p_spawn(orphan_child, argv, STDERR_FILENO, STDERR_FILENO, "orphan_child");
    p_exit();
}

void bg(pid_t pid) {
    p_bg(pid);
    p_exit();
}

void fg(pid_t pid) {
    p_fg(pid);
    p_exit();
}

void bash_mount(const char *fs_name, int *status) {
    if (f_mount(fs_name) == -1) {
        *status = -1;
    } else {
        *status = 0;
    }
}

void bash_touch(struct parsed_command *cmd) {
    f_touch(cmd);
    p_exit();
}

void bash_rm(const char *fs_name) {
    f_rm(fs_name);
    p_exit();
}

void bash_mv(const char *src, const char *dst) {
    f_mv(src, dst);
    p_exit();
}

void bash_cp(struct parsed_command *cmd) {
    f_cp(cmd);
    p_exit();
}

void bash_cat(struct parsed_command *cmd) {
    f_cat(cmd);
    p_exit();
}

void bash_ls() {
    f_ls();
    p_exit();
}

void bash_chmod(const char* mode, const char* fs_name) {
    f_chmod(mode, fs_name);
    p_exit();
}

void print_busy() {
    int i = 0;
    while(1) {
        if (i % 100000 == 0) {
            printf("%d\n", i);
        } if (i % 200000 == 0) {
            break;
        }
        i++;
    }
    p_exit();
}

void egg() {
    p_perror("You found the easter egg! You should not be calling this function here, just in the standalone :)", CommandNotFoundError);
    p_exit();
}