#include <string.h>
#include <time.h>

#include "parser.h"
#include "bash.h"
#include "stress.h"
#include <time.h>
#include "Deque_PID.h"
#include <termios.h>

#define MAX_LINE_LENGTH 4096
#define NUM_CMDS 27
pid_t shell_pid = 2;
FILE* logFile;

pid_t fg_pid;
int current_background_id = 0; //background id
int background_list[MAX_LINE_LENGTH]; //given pid_t, return background id
int ec = false;

//sigint handler
void register_sigint_handler();
void sigint_handler(int signum) {
    if (fg_pid > shell_pid) {
        p_kill(fg_pid, S_SIGTERM);
    }
    register_sigint_handler();
}
void register_sigint_handler() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sa.sa_handler = sigint_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, 0);
}

//sigquit handler
void register_sigquit_handler();
void sigquit_handler(int signum) {
    if (fg_pid > shell_pid) {
        p_kill(fg_pid, S_SIGTERM);
    }
    register_sigquit_handler();
}
void register_sigquit_handler() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sa.sa_handler = sigquit_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGQUIT, &sa, 0);
}

//sigtstp handler
void register_sigtstp_handler();
void sigtstp_handler(int signum) {
    if (fg_pid > shell_pid) {
        p_kill(fg_pid, S_SIGSTOP);
    }
    register_sigtstp_handler();
}
void register_sigtstp_handler() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTSTP);
    sa.sa_handler = sigtstp_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, 0);
}


//function array
void (*func_array[])() = { egg, bash_sleep, busy, bash_echo, bash_kill, zombify, orphanify, bash_ps, bash_nice, nice_pid, jobs, fg, bg, egg, egg,
    egg, egg, bash_touch, bash_rm, bash_mv, bash_cp, bash_cat, bash_ls, bash_chmod, nohang, hang, recur};

//function descriptions for man command array
const char *func_names[] = { 
    "man (S) list all available commands.",
    "sleep n (S*) sleep for n seconds.", 
    "busy (S*) busy wait indefinitely.", 
    "echo (S*) similar to echo(1) in the VM.", 
    "kill [ -SIGNAL_NAME ] pid ... (S*) send the specified signal to the specified processes, where -SIGNAL_NAME is either term (the default), stop, or cont, corresponding to S_SIGTERM, S_SIGSTOP, and S_SIGCONT, respectively. Similar to /bin/kill in the VM.", 
    "zombify (S*) spawns a process and while(1)s indefinitely, testing our zombie process handling",
    "orphanify (S*) spawns a process that while(1)s indefinitely, testing our orphan process handling", 
    "ps (S*) list all processes on PennOS. Display pid, ppid, and priority.", 
    "nice priority command [arg] (S) set the priority of the command to priority and execute the command.", 
    "nice_pid priority pid (S) adjust the nice level of process pid to priority priority.", 
    "jobs (S) list all jobs.", 
    "logout (S) exit the shell and shutdown PennOS.",
    "fg [job_id] (S) bring the last stopped or backgrounded job to the foreground, or the job specified by job_id.", 
    "bg [job_id] (S) continue the last stopped job, or the job specified by job_id. Note that this does mean you will need to implement the & operator in your shell.", 
    "mkfs FS_NAME BLOCKS_IN_FAT BLOCK_SIZE_CONFIG Creates a PennFAT filesystem in the file named FS_NAME. The number of blocks in the FAT region is BLOCKS_IN_FAT (ranging from 1 through 32), and the block size is 256, 512, 1024, 2048, or 4096 bytes corresponding to the value (0 through 4) of BLOCK_SIZE_CONFIG.",
    "mount FS_NAME Mounts the filesystem named FS_NAME by loading its FAT into memory.", 
    "umount Unmounts the currently mounted filesystem.", 
    "touch file ... (S*) create an empty file if it does not exist, or update its timestamp otherwise.", 
    "mv SOURCE DEST Renames SOURCE to DEST.", 
    "rm FILE ... Removes the files.",
    "cp src dest (S*) copy src to dest", 
    "cat (S*) The usual cat from bash, etc.", 
    "ls (S*) list all files in the working directory (similar to ls -il in bash), same formatting as ls in the standalone PennFAT.", 
    "chmod (S*) similar to chmod(1) in the VM",
    "nohang (S) uses Stress.c to test our p_waitpid function with nohang", 
    "hang (S) uses Stress.c to test our p_waitpid function with nohang", 
    "recur (S) uses Stress.c to test our p_waitpid function that spawns generations A-Z and reaps accordingly"
};

// returns a negative if the function takes in the parsed cmd struct as input
int get_func_idx(char* name_str) {
    if (strcmp(name_str, "sleep") == 0) {
        return 1;
    } else if (strcmp(name_str, "busy") == 0) {
        return 2;
    } else if (strcmp(name_str, "echo") == 0) {
        return -3;
    } else if (strcmp(name_str, "kill") == 0) {
        return 4;
    } else if (strcmp(name_str, "zombify") == 0) {
        return 5;
    } else if (strcmp(name_str, "orphanify") == 0) {
        return 6;
    } else if (strcmp(name_str, "ps") == 0) {
        return 7;
    } else if (strcmp(name_str, "nice") == 0) {
        return 8;
    } else if (strcmp(name_str, "nice_pid") == 0) {
        return 9;
    } else if (strcmp(name_str, "jobs") == 0) {
        return 10;
    } else if (strcmp(name_str, "fg") == 0) {
        return 11;
    } else if (strcmp(name_str, "bg") == 0) {
        return 12;
    } else if (strcmp(name_str, "print_busy") == 0) {
        return 13;
    } else if (strcmp(name_str, "mkfs") == 0) {
        return 14;
    } else if (strcmp(name_str, "mount") == 0) {
        return 15;
    } else if (strcmp(name_str, "umount") == 0) {
        return 16;
    } else if (strcmp(name_str, "touch") == 0) {
        return -17;
    } else if (strcmp(name_str, "rm") == 0) {
        return 18;
    } else if (strcmp(name_str, "mv") == 0) {
        return 19;
    } else if (strcmp(name_str, "cp") == 0) {
        return -20;
    } else if (strcmp(name_str, "cat") == 0) {
        return -21;
    } else if (strcmp(name_str, "ls") == 0) {
        return 22;
    } else if (strcmp(name_str, "chmod") == 0) {
        return 23;
    } else if (strcmp(name_str, "nohang") == 0) {
        return 24;
    } else if (strcmp(name_str, "hang") == 0) {
        return 25;
    } else if (strcmp(name_str, "recur") == 0) {
        return 26;
    } else {
        return -100;
    }
}

FILE* setUpNoncanonicalMode() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &new_termios);

    // Disable canonical mode (line buffering) and echo
    new_termios.c_lflag &= ~(ICANON | ECHO);

    // Set minimum characters for noncanonical read
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    tcflush(STDIN_FILENO, TCIFLUSH);

    FILE* history = fopen("history/cmd_history.txt", "w+");
    if (!history) {
        p_perror("Error opening history file", FileIsOpenError);
    }
    return history;
}

void write_to_history(FILE* history, char* cmd) {
    // since we are writing to a file in our local os, and not in our fs, we use fprintf
    fprintf(history, "%s", cmd);
}

void read_history(FILE *file, char history[20][20], int *historySize) {
    f_seek(file, 0, SEEK_SET); // Move to the beginning of the file
    *historySize = 0;

    while (*historySize < 100 && f_gets(history[*historySize], MAX_LINE_LENGTH, file) != NULL) {
        // Remove newline character at the end
        size_t length = strlen(history[*historySize]);
        if (history[*historySize][length - 1] == '\n') {
            history[*historySize][length - 1] = '\0';
        }
        (*historySize)++;
    }
}

void run_processes(pid_Deque* curr_shell_pids, int num_bytes, char* file_name, const char* fs_in, const char* fs_out) {
    struct parsed_command* cmd;
    directory_entry dir_entry;
    int status;
    int offset = f_find_file(file_name, &dir_entry);
    if (offset == -1) {
        p_perror("File not found", FileNotFoundError);
        return;
    }
    if (!(dir_entry.perm & 1)) {
        p_perror("File does not have executable permission", PermissionError);
        return;
    }

    // get contents of the file 
    char file_contents[dir_entry.size];
    int fd = f_open(file_name, F_READ);
    num_bytes = f_read(fd, dir_entry.size, file_contents);
    f_close(fd);
    
    char* raw_input = f_strtok(file_contents, "\n");
    while (raw_input != NULL) {
        parse_command(raw_input, &cmd);
        raw_input = f_strtok(NULL, "\n");
        if (raw_input != NULL) {
            num_bytes = strlen(raw_input);
        } else {
            num_bytes = 0;
        }
        while (1) {
            pid_t waited_pid = p_waitpid(-1, &status, true);
            if (waited_pid == -1 || waited_pid == 0) {
                break;
            }
            char* waited_pid_name = get_pcb_name_from_pid(waited_pid);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                p_background_wait(-1, background_list[(int)waited_pid], waited_pid_name);
                pid_Deque_Pop_PID(curr_shell_pids, waited_pid);
            } else if (WIFSTOPPED(status)) {
                p_background_wait(0, background_list[(int)waited_pid], waited_pid_name);
                pid_Deque_Pop_PID(curr_shell_pids, waited_pid);
                pid_Deque_Push_Back(curr_shell_pids, waited_pid);
            }
        }


        // Skip on Empty String
        if (cmd->num_commands == 0 || cmd->commands[0][0] == NULL || strcmp(cmd->commands[0][0], "\n") == 0 || num_bytes == 0) {  
            continue;
        }

        //increment current_background_id
        if (cmd->is_background) {
            current_background_id++;
        } 

        // nice priority command
        if (strcmp(cmd->commands[0][0], "nice") == 0) {
            int priority;
            priority = atoi(cmd->commands[0][1]);

            int func_idx = get_func_idx(cmd->commands[0][2]);

            if (func_idx == -1) {
                p_perror("Invalid function", CommandNotFoundError);
                continue;
            }
            char* argv[] = { cmd->commands[0][3], NULL };
            
            int c_pid = p_spawn(func_array[func_idx], argv, STDIN_FILENO, STDOUT_FILENO, cmd->commands[0][2]);
            if (c_pid < 0) {
                p_perror("p_spawn", ProcessSpawnError);
            }
            
            p_nice(c_pid, priority);
            
            // only wait for a fg process
            if (cmd->is_background) {
                background_list[(int)c_pid] = current_background_id - 1;
                p_background_status(current_background_id, c_pid);
                continue;
            } else {
                fg_pid = c_pid;
                // only parent executing at this point, child executed func
                p_waitpid(fg_pid, &status, false);
                // control handler will handle if its moved to bg
                char* waited_pid_name = get_pcb_name_from_pid(fg_pid);
                if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                    pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
                }
            }
            continue;

        } else if (strcmp(cmd->commands[0][0], "man") == 0) {
            for (size_t i = 0; i < NUM_CMDS; i++) {
                f_fprintf(stderr, "%s\n", func_names[i]);
            }
            continue;
        } else if (strcmp(cmd->commands[0][0], "jobs") == 0) {
            p_jobs();
            continue;
        } else if (strcmp(cmd->commands[0][0], "nice_pid") == 0) {
            int priority = atoi(cmd->commands[0][1]);
            int pid = atoi(cmd->commands[0][2]);
            p_nice(pid, priority);
            continue;
        } else if (strcmp(cmd->commands[0][0], "logout") == 0) {
            //logout
            p_logout();
            return;
        } else if (strcmp(cmd->commands[0][0], "bg") == 0) {
            pid_t bg_pid = -1;
            if (cmd->commands[0][1]) {
                bg_pid = atoi(cmd->commands[0][1]);
            }
            bg_pid = p_bg(bg_pid);
            if (bg_pid == -1) {
                p_perror("No stopped processes", ProcessNotFoundError);
            }
            continue;
        } else if (strcmp(cmd->commands[0][0], "fg") == 0) {
            fg_pid = -1;
            if (cmd->commands[0][1]) {
                fg_pid = atoi(cmd->commands[0][1]);
            } else {
                pid_DequeNode* fg_node = curr_shell_pids->back;
                // print out fg deque first
                while(fg_node) {
                    fg_node = fg_node->prev;
                }

                fg_node = curr_shell_pids->back;
                // check to make sure pid is still running
                while (fg_node) {
                    // get first background running or stopped job to the foreground
                    p_waitpid(fg_node->pid, &status, true);
                    if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                        // if exited, remove from queue
                        pid_Deque_Pop_PID(curr_shell_pids, fg_node->pid);
                    } else {
                        // stopped or still runnning
                        fg_pid = fg_node->pid;
                        if (W_WIFSTOPPED(status)) {
                            p_kill(fg_pid, S_SIGCONT);
                        }
                        break;
                    }
                    fg_node = fg_node->prev;
                }
            }
            pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
            fg_pid = p_fg(fg_pid);
            if (fg_pid == -1) {
                p_perror("No stopped/backgrounded processes", ProcessNotFoundError);
                continue;
            }
            // continue to end where we wait for fg_pid
            // printf("[fg] fgpid right before wait: %d\n", fg_pid);
            // only parent executing at this point, child executed func
            p_waitpid(fg_pid, &status, false);
            if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
            } else {
                // if it was stopped again, move it to the back
                pid_Deque_Push_Back(curr_shell_pids, fg_pid);
            }
            
        }
        else if (strcmp(cmd->commands[0][0], "sleep") == 0) {
            int sleep_ticks = 0;
            if (atoi(cmd->commands[0][1]) != 0) {
                sleep_ticks = atoi(cmd->commands[0][1]) * CLOCKS_PER_SEC;
            } else {
                p_perror("Sleep arguments", ArgumentNotFoundError);
            }
            pid_t sleep_pid  = p_sleep(sleep_ticks);

            // waiting
            if (sleep_pid < 0) {
                p_perror("p_spawn error", ProcessSpawnError);
            } else {
                // add new process to curr_shell_pids deque
                pid_Deque_Push_Back(curr_shell_pids, sleep_pid);

                // only wait for a fg process
                if (cmd->is_background) {
                    background_list[(int)sleep_pid] = current_background_id - 1;
                    p_background_status(current_background_id, sleep_pid);
                    continue;
                } else {
                    fg_pid = sleep_pid;
                    // only parent executing at this point, child executed func
                    p_waitpid(fg_pid, &status, false);

                    if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                        pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
                    } 
                }
            }
            continue;
        } else {
            int func_idx = get_func_idx(cmd->commands[0][0]);
            int c_pid;

            // spawn based on implementation argument types
            if (func_idx == -100) {
                p_perror("Invalid function", CommandNotFoundError);
                continue;
            } else if (func_idx < 0) {
                int fdx_in = STDIN_FD;
                int fdx_out = STDOUT_FD;

                if (fs_in != NULL) {
                    fdx_in = f_open(fs_in, F_READ);
                } else if (fs_out != NULL) {
                    fdx_out = f_open(fs_out, F_APPEND);
                }

                if (cmd->stdin_file != NULL) {
                    fdx_in = f_open(cmd->stdin_file, F_READ);
                }
                if (cmd->stdout_file != NULL && cmd->is_file_append) {
                    fdx_out = f_open(cmd->stdout_file, F_APPEND);
                } else if (cmd->stdout_file != NULL) {
                    fdx_out = f_open(cmd->stdout_file, F_WRITE);
                }

                c_pid = p_spawn_cmd(func_array[-func_idx], cmd, fdx_in, fdx_out, *cmd->commands[0]);
            } else {
                int fdx_in = STDIN_FD;
                int fdx_out = STDOUT_FD;
                
                if (fs_in != NULL) {
                    fdx_in = f_open(fs_in, F_READ);
                } else if (fs_out != NULL) {
                    fdx_out = f_open(fs_out, F_APPEND);
                }

                if (cmd->stdin_file != NULL) {
                    fdx_in = f_open(cmd->stdin_file, F_READ);
                }
                if (cmd->stdout_file != NULL && cmd->is_file_append) {
                    fdx_out = f_open(cmd->stdout_file, F_APPEND);
                } else if (cmd->stdout_file != NULL) {
                    fdx_out = f_open(cmd->stdout_file, F_WRITE);
                }

                char* argv[] = { cmd->commands[0][1], cmd->commands[0][2], cmd->commands[0][3], NULL };
                c_pid = p_spawn(func_array[func_idx], argv, fdx_in, fdx_out, *cmd->commands[0]);
            }

            // waiting
            if (c_pid < 0) {
                p_perror("Process spawn", ProcessSpawnError);
            } else {
                // add new process to shell queue
                pid_Deque_Push_Back(curr_shell_pids, c_pid);
                // only wait for a fg process
                if (cmd->is_background) {
                    background_list[(int)c_pid] = current_background_id - 1;
                    p_background_status(current_background_id, c_pid);
                    continue;
                } else {
                    fg_pid = c_pid;
                    // only parent executing at this point, child executed func
                    p_waitpid(fg_pid, &status, false);

                    // signal handler will handle if its moved to bg
                    if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                        // check if child had oopen fds
                        // if yes close bth4em 
                        pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
                    } else {
                    }
                }
            }
        }
    }
}

void shell() {
    struct parsed_command* cmd;
    // deque of currently running or stopped processes (i.e. unterminated / not waited on), not including the shell
    // the last item is most recent one. when a process is stop its moved to the back of the queue
    pid_Deque* curr_shell_pids = pid_Deque_Allocate();
    if (!curr_shell_pids) {
        // printf("Error: allocating deque\n");
        p_perror("Deque Allocation", DequeSpaceError);
    }
    int status;
    char history[20][20];
    int historySize = 0;
    int currentHistoryPosition = 0;
    bool esc = false;
    FILE* file;

    if (ec) {
        file = setUpNoncanonicalMode();
    }

    while (1) {
        char raw_input[MAX_LINE_LENGTH];
        raw_input[0] = '\0';
        // foreground is shell again
        fg_pid = shell_pid;

        int status;
        while (1) {
            pid_t waited_pid = p_waitpid(-1, &status, true);
            if (waited_pid == -1 || waited_pid == 0) {
                break;
            }
            char* waited_pid_name = get_pcb_name_from_pid(waited_pid);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                p_background_wait(-1, background_list[(int)waited_pid], waited_pid_name);
                pid_Deque_Pop_PID(curr_shell_pids, waited_pid);
            } else if (WIFSTOPPED(status)) {
                p_background_wait(0, background_list[(int)waited_pid], waited_pid_name);
                pid_Deque_Pop_PID(curr_shell_pids, waited_pid);
                pid_Deque_Push_Back(curr_shell_pids, waited_pid);
            }
        }

        if (f_write(STDOUT_FILENO, PROMPT, strlen(PROMPT)) == -1) {
            p_perror("Error in writing prompt", PromptError);
        }

        int num_bytes;
        directory_entry temp_entry;
        if (ec) {
            num_bytes = 0;
            read_history(file, history, &historySize);
            currentHistoryPosition = historySize;
            while (1) {
                char c;
                f_read(STDIN_FILENO, 1, &c);
                if (c == '\x04') {
                    // CTRL-D / EOF
                    fclose(logFile);
                    return;
                }
                if (c == '\x1b') {
                    f_read(STDIN_FILENO, 1, &c);
                    if (c == '[') {
                        f_read(STDIN_FILENO, 1, &c);
                        // process the entire history file
                        if (c == 'A') {
                            // read everything from history
                            if (currentHistoryPosition > 0) {
                                currentHistoryPosition--;
                            }
                            f_write(STDOUT_FILENO, "\033[2K\rPennOS> ", 13);
                            f_write(STDOUT_FILENO, history[currentHistoryPosition], strlen(history[currentHistoryPosition]));
                            strcpy(raw_input, history[currentHistoryPosition]);
                            num_bytes = strlen(raw_input);
                            esc = true;
                        } else if (c == 'B') {
                            // read the previous line from history
                            if (currentHistoryPosition < historySize - 1) {
                                currentHistoryPosition++;
                            }
                            if (currentHistoryPosition < historySize) {
                                f_write(STDOUT_FILENO, "\033[2K\rPennOS> ", 13);
                                f_write(STDOUT_FILENO, history[currentHistoryPosition], strlen(history[currentHistoryPosition]));
                                strcpy(raw_input, history[currentHistoryPosition]);
                                num_bytes = strlen(raw_input);
                                esc = true;
                            }
                        }
                    }
                } else if (c == '\n') {
                    raw_input[num_bytes] = '\n';
                    raw_input[num_bytes + 1] = '\0';
                    f_write(STDOUT_FILENO, &c, 1);
                    num_bytes++;
                    currentHistoryPosition = historySize - 1;
                    break;
                } else if (c == '\x7f') {
                    // Handle backspace
                    if (num_bytes > 0) {
                        // Simulate backspace by moving cursor back and clearing the character
                        f_write(STDOUT_FILENO, "\b \b", 3);
                        // Remove the last character from the buffer
                        raw_input[num_bytes - 1] = '\0';
                        num_bytes -= 1;  // Adjust for the removed character and the current backspace
                    }
                } else {
                    raw_input[num_bytes] = c;
                    f_write(STDOUT_FILENO, &c, 1);
                    num_bytes++;
                }
            }
            if (esc) {
                esc = false;
            } else {
                write_to_history(file, raw_input);
            }
        } else {
            num_bytes = f_read(STDIN_FILENO, MAX_LINE_LENGTH, raw_input);
            if (num_bytes == -1) {
                p_perror("Error in reading command", CommandNotFoundError);
            }
        }


        while (1) {
            pid_t waited_pid = p_waitpid(-1, &status, true);
            if (waited_pid == -1 || waited_pid == 0) {
                break;
            }
            char* waited_pid_name = get_pcb_name_from_pid(waited_pid);
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                p_background_wait(-1, background_list[(int)waited_pid], waited_pid_name);
                pid_Deque_Pop_PID(curr_shell_pids, waited_pid);
            } else if (WIFSTOPPED(status)) {
                p_background_wait(0, background_list[(int)waited_pid], waited_pid_name);
                pid_Deque_Pop_PID(curr_shell_pids, waited_pid);
                pid_Deque_Push_Back(curr_shell_pids, waited_pid);
            }
        }


        if (num_bytes == -1) {
            p_perror("Error in reading command", CommandNotFoundError);
        } else if (num_bytes == 0) {
            // TODO: CTRL-D / EOF, want to exit entire shell/entire OS process??
            fclose(logFile);
            p_logout();
        }
        raw_input[num_bytes] = '\0';

        // remove newline from cmd
        if (num_bytes > 0 && raw_input[num_bytes-1] == '\n') {
            raw_input[num_bytes-1] = '\0';
        }

        int parse_debug = parse_command(raw_input, &cmd);

        if (parse_debug < 0) {
            p_perror("Invalid function", CommandNotFoundError);
            p_logout();
        }
        if (parse_debug > 0) {
            p_perror("Invalid function", CommandNotFoundError);
            continue;
        }

        // Reprompt on Empty String
        if (cmd->num_commands == 0) {
            continue;
        }

        //increment current_background_id
        if (cmd->is_background) {
            current_background_id++;
        } 

        // nice priority command
        if (strcmp(cmd->commands[0][0], "nice") == 0) {
            int priority;
            priority = atoi(cmd->commands[0][1]);

            int func_idx = get_func_idx(cmd->commands[0][2]);

            if (func_idx == -1) {
                p_perror("Invalid function", CommandNotFoundError);
                continue;
            }
            char* argv[] = { cmd->commands[0][3], NULL };
            
            int c_pid = p_spawn(func_array[func_idx], argv, STDIN_FILENO, STDOUT_FILENO, cmd->commands[0][2]);
            if (c_pid < 0) {
                p_perror("p_spawn", ProcessSpawnError);
            }
            
            p_nice(c_pid, priority);
            
            // only wait for a fg process
            if (cmd->is_background) {
                background_list[(int)c_pid] = current_background_id - 1;
                p_background_status(current_background_id, c_pid);
                continue;
            } else {
                fg_pid = c_pid;
                // only parent executing at this point, child executed func
                p_waitpid(fg_pid, &status, false);
                // control handler will handle if its moved to bg
                char* waited_pid_name = get_pcb_name_from_pid(fg_pid);
                if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                    pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
                } else {
                    // if stopped, its already at the back of the queue
                }
            }
            continue;

        } else if (strcmp(cmd->commands[0][0], "man") == 0) {
            for (size_t i = 0; i < NUM_CMDS; i++) {
                f_fprintf(stderr, "%s\n", func_names[i]);
            }
            continue;
        } else if (strcmp(cmd->commands[0][0], "jobs") == 0) {
            p_jobs();
            continue;
        } else if (strcmp(cmd->commands[0][0], "nice_pid") == 0) {
            int priority = atoi(cmd->commands[0][1]);
            int pid = atoi(cmd->commands[0][2]);
            p_nice(pid, priority);
            continue;
        } else if (strcmp(cmd->commands[0][0], "logout") == 0) {
            //logout
            p_logout();
            return;
        } else if (strcmp(cmd->commands[0][0], "bg") == 0) {
            pid_t bg_pid = -1;
            if (cmd->commands[0][1]) {
                bg_pid = atoi(cmd->commands[0][1]);
            }
            bg_pid = p_bg(bg_pid);
            if (bg_pid == -1) {
                p_perror("No stopped processes", ProcessNotFoundError);
            }
            continue;
        } else if (strcmp(cmd->commands[0][0], "fg") == 0) {
            fg_pid = -1;
            if (cmd->commands[0][1]) {
                fg_pid = atoi(cmd->commands[0][1]);
            } else {
                pid_DequeNode* fg_node = curr_shell_pids->back;
                // print out fg deque first
                while(fg_node) {
                    fg_node = fg_node->prev;
                }

                fg_node = curr_shell_pids->back;
                // check to make sure pid is still running
                while (fg_node) {
                    // get first background running or stopped job to the foreground
                    p_waitpid(fg_node->pid, &status, true);
                    if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                        // if exited, remove from queue
                        pid_Deque_Pop_PID(curr_shell_pids, fg_node->pid);
                    } else {
                        // stopped or still runnning
                        fg_pid = fg_node->pid;
                        if (W_WIFSTOPPED(status)) {
                            p_kill(fg_pid, S_SIGCONT);
                        }
                        break;
                    }
                    fg_node = fg_node->prev;
                }
            }
            pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
            fg_pid = p_fg(fg_pid);
            if (fg_pid == -1) {
                p_perror("No stopped/backgrounded processes", ProcessNotFoundError);
                continue;
            }
            // continue to end where we wait for fg_pid
            // printf("[fg] fgpid right before wait: %d\n", fg_pid);
            // only parent executing at this point, child executed func
            p_waitpid(fg_pid, &status, false);

            if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                // printf("exited\n");
            } else {
                // printf("stopped [shell check]\n");
                // if it was stopped again, move it to the back
                pid_Deque_Push_Back(curr_shell_pids, fg_pid);
            }
            
        }
        else if (strcmp(cmd->commands[0][0], "sleep") == 0) {
            int sleep_ticks = 0;
            if (atoi(cmd->commands[0][1]) != 0) {
                sleep_ticks = atoi(cmd->commands[0][1]) * CLOCKS_PER_SEC;
            } else {
                p_perror("Sleep arguments", ArgumentNotFoundError);
            }
            pid_t sleep_pid  = p_sleep(sleep_ticks);

            // waiting
            if (sleep_pid < 0) {
                p_perror("p_spawn error", ProcessSpawnError);
            } else {
                // add new process to curr_shell_pids deque
                pid_Deque_Push_Back(curr_shell_pids, sleep_pid);

                // only wait for a fg process
                if (cmd->is_background) {
                    background_list[(int)sleep_pid] = current_background_id - 1;
                    p_background_status(current_background_id, sleep_pid);
                    continue;
                } else {
                    fg_pid = sleep_pid;
                    // only parent executing at this point, child executed func
                    p_waitpid(fg_pid, &status, false);
                    // control handler will handle if its moved to bg

                    if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                        // printf("exited normally\n");
                        pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
                    } else {
                    }

                }
            }
            continue;
        } else if (f_find_file(cmd->commands[0][0], &temp_entry) != -1) {
            run_processes(curr_shell_pids, num_bytes, cmd->commands[0][0], cmd->stdin_file, cmd->stdout_file);
            continue;
        } else {
            int func_idx = get_func_idx(cmd->commands[0][0]);
            int c_pid;

            // spawn based on implementation argument types
            if (func_idx == -100) {
                p_perror("Invalid function", CommandNotFoundError);
                continue;
            } else if (func_idx < 0) {
                // func_idx *= -1;
                int fd_in = STDIN_FILENO;
                int fd_out = STDOUT_FILENO;
                if (cmd->stdin_file != NULL) {
                    fd_in = f_open(cmd->stdin_file, F_READ);
                }
                if (cmd->stdout_file != NULL && cmd->is_file_append) {
                    fd_out = f_open(cmd->stdout_file, F_APPEND);
                } else if (cmd->stdout_file != NULL) {
                    fd_out = f_open(cmd->stdout_file, F_WRITE);
                }

                c_pid = p_spawn_cmd(func_array[-func_idx], cmd, fd_in, fd_out, *cmd->commands[0]);
            } else {
                int fd_in = STDIN_FILENO;
                int fd_out = STDOUT_FILENO;
                if (cmd->stdin_file != NULL) {
                    fd_in = f_open(cmd->stdin_file, F_READ);
                }
                if (cmd->stdout_file != NULL && cmd->is_file_append) {
                    fd_out = f_open(cmd->stdout_file, F_APPEND);
                } else if (cmd->stdout_file != NULL) {
                    fd_out = f_open(cmd->stdout_file, F_WRITE);
                }

                char* argv[] = { cmd->commands[0][1], cmd->commands[0][2], cmd->commands[0][3], NULL };
                c_pid = p_spawn(func_array[func_idx], argv, fd_in, fd_out, *cmd->commands[0]);

            }

            // waiting
            if (c_pid < 0) {
                p_perror("Process spawn", ProcessSpawnError);
            } else {
                // add new process to shell queue
                pid_Deque_Push_Back(curr_shell_pids, c_pid);
                // only wait for a fg process
                if (cmd->is_background) {
                    background_list[(int)c_pid] = current_background_id - 1;
                    // printf("[%d]\t%d\n", current_background_id, c_pid);
                    p_background_status(current_background_id, c_pid);
                    continue;
                } else {
                    fg_pid = c_pid;
                    // only parent executing at this point, child executed func
                    p_waitpid(fg_pid, &status, false);
                    // signal handler will handle if its moved to bg
                    if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
                        pid_Deque_Pop_PID(curr_shell_pids, fg_pid);
                    }
                }
            }
        }
    }
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Provide a file system\n");
        return -1; 
    }

    if (argc == 3 && strcmp(argv[2], "-ec") == 0) {
        ec = true;
    }

    int status = 0;
    bash_mount(argv[1], &status);
    if (status == -1) {
        printf("Error mounting file system\n");
        return -1;
    }

    logFile = fopen("log/log.txt", "w+");
    
    register_sigint_handler();
    register_sigquit_handler();
    register_sigtstp_handler();

    p_system_init(shell);

    start_os();
    
    return 0;
}

// call just main scheduler function with loop for now