#include "k_pennos.h"
#include <valgrind/valgrind.h>
#include <time.h>
#include "signals.h"
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include "f_pennos.h"
#include <stdarg.h>

// error macros
#define ERRNO errno
#define MAX_LINE_LENGTH 4096

// system init upon booting shell?
extern Deque* global_pcbs;
extern pid_t current_pid;
extern PCB* current_pcb;
extern FILE* logFile;

// Extern variables from pennfat.c
extern int fs_fd;
extern uint16_t *fat;
extern size_t fat_size;
extern int block_size;

FileDescriptor fd_table[MAX_OPEN_FILES];

int update_fs_dir_entry(directory_entry dir_entry, off_t position) {
    int offset = lseek(fs_fd, position, SEEK_SET);
    if (offset == -1) {
        return -1;
    }
    int write_bytes = write(fs_fd, &dir_entry, sizeof(directory_entry));
    if (write_bytes == -1) {
        return -1;
    }
    return 0;
}

int find_global_open_fd() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (fd_table[i].fd_type == FD_UNINIT) {
            return i;
        }
    }
    return -1;
}

int find_fd_in_pcb(int fd) {
    if (current_pcb == NULL) {
        return -1;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (current_pcb->open_fds[i] == fd) {
            return i;
        }
    }

    return -1;
}

int add_fd_to_pcb(int global_i) {
    if (current_pcb == NULL) {
        return -1;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (current_pcb->open_fds[i] == -1) {
            current_pcb->open_fds[i] = global_i;
            current_pcb->num_open_fds++;
            return i;
        }
    }
    return -1;
}

int find_fat() {
    int open_fat_value = -1;
    int blocks_in_fat = fat_size / block_size;
    int num_fat_entries = block_size * blocks_in_fat / 2;

    for (int i = 1; i < num_fat_entries; i++) {
        if (fat[i] == 0) {
            open_fat_value = i;
            break;
        }
    }
    
    return open_fat_value;
}

int f_open(const char *fname, int mode) {
    // Check if file exists
    directory_entry dir_entry;
    int position;
    position = find_file(fname, &dir_entry);
    if (position == -1 && mode == F_READ) { // Error if file not found and read mode
        p_perror("File not found", FileNotFoundError);
        return -1;
    }

    // First check if file is already on the global table, and set if possible
    int global_index = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (strcmp(fname, fd_table[i].dir_entry.name) == 0) {
            global_index = i;
            break;
        }
    }

    switch (mode) {
        // F_READ, check if file has read permissions
        case F_READ: {
            // If file was not on global table, add to global, update pcd fd table, and return global index
            if (global_index == -1) {
                // Check that global has space
                if ((global_index = find_global_open_fd()) == -1) {
                    p_perror("Reached global fd limit", TooManyFilesOpenError);
                    return -1;
                }

                // Check that file has read permissions
                if (!(dir_entry.perm & 4)) {
                    p_perror("Permission denied", PermissionError);
                    return -1;
                }
                
                fd_table[global_index].dir_entry = dir_entry;
                fd_table[global_index].fd_type = FD_FILE;
                fd_table[global_index].mode = F_READ;
                fd_table[global_index].offset = 0;
                fd_table[global_index].ref_count = 1;
            } else { // If file was on global, just check if the file has read permission and update ref_count
                if (!(fd_table[global_index].dir_entry.perm & 4)) {
                    p_perror("Permission denied", PermissionError);
                    return -1;
                }
                fd_table[global_index].ref_count += 1;
            }

            int pcb_index;
            if ((pcb_index = add_fd_to_pcb(global_index)) == -1) { // Add if not
                p_perror("Error adding fd to pcb", TooManyFilesOpenError);
                return -1;
            }

            return pcb_index;
        }
        case F_WRITE: {
            if (global_index == -1) { // Not in global index
                if (position == -1) { // Create the file if it doesn't exist
                    if(touch_single(fname) == -1) {
                        p_perror("Error creating file", FileNotFoundError);
                        return -1;
                    }
                    position = find_file(fname, &dir_entry);
                } else { // File exists, check if file has write permissions, truncate
                    if (!(dir_entry.perm & 2)) {
                        p_perror("Permission denied", PermissionError);
                        return -1;
                    }
                    rm(fname);
                    touch_single(fname);
                    position = find_file(fname, &dir_entry);
                }

                // Check if global has space
                if ((global_index = find_global_open_fd()) == -1) {
                    p_perror("Reached global fd limit", TooManyFilesOpenError);
                    return -1;
                }

                // Add to global table
                fd_table[global_index].dir_entry = dir_entry;
                fd_table[global_index].fd_type = FD_FILE;
                fd_table[global_index].mode = F_WRITE;
                fd_table[global_index].offset = 0;
                fd_table[global_index].ref_count = 1;
            } else { // File is in global table
                // If there are no write permissions or file is already open in write mode, error
                if (!(fd_table[global_index].dir_entry.perm & 2) || fd_table[global_index].mode == F_WRITE) {
                    p_perror("Permission denied", PermissionError);
                    return -1;
                }
                rm(fname);
                touch_single(fname);
                position = find_file(fname, &dir_entry);
                fd_table[global_index].dir_entry = dir_entry;
                fd_table[global_index].mode = F_WRITE;
                fd_table[global_index].ref_count += 1;
            }

            int pcb_index;
            if ((pcb_index = add_fd_to_pcb(global_index)) == -1) { // Add if not
                p_perror("Error adding fd to pcb", TooManyFilesOpenError);
                return -1;
            }
            return pcb_index;
        }
        case F_APPEND: {
            if (global_index == -1) { // Not in global index
                if (position == -1) { // Create the file if it doesn't exist
                    if(touch_single(fname) == -1) {
                        p_perror("Error creating file", FileNotFoundError);
                        return -1;
                    }
                    position = find_file(fname, &dir_entry);
                } else {
                    if (!(dir_entry.perm & 2)) {
                        p_perror("Permission denied", PermissionError);
                        return -1;
                    }
                }

                // Check if global has space
                if ((global_index = find_global_open_fd()) == -1) {
                    p_perror("Reached global fd limit", TooManyFilesOpenError);
                    return -1;
                }

                // Add to global table
                fd_table[global_index].dir_entry = dir_entry;
                fd_table[global_index].fd_type = FD_FILE;
                fd_table[global_index].mode = F_APPEND;
                fd_table[global_index].offset = dir_entry.size; // Set offset to end of file
                fd_table[global_index].ref_count = 1;
            } else { // File is in global table
                // If there are no write permissions, error
                if (!(fd_table[global_index].dir_entry.perm & 2)) {
                    p_perror("Permission denied", PermissionError);
                    return -1;
                }
                fd_table[global_index].mode = F_APPEND;
                fd_table[global_index].offset = dir_entry.size;
                fd_table[global_index].ref_count += 1;
            }

            int pcb_index;
            if ((pcb_index = add_fd_to_pcb(global_index)) == -1) { // Add if not
                p_perror("Error adding fd to pcb", TooManyFilesOpenError);
                return -1;
            }
            return pcb_index;
        }
    }
    p_perror("Invalid argument: mode", ArgumentNotFoundError);
    return -1;
}

int f_close(int fd) {
    if (fd > MAX_OPEN_FILES || (current_pcb->open_fds[fd] == -1) || fd < 0) {
        p_perror("Invalid file descriptor", InvalidFileDescriptorError);
        return -1;
    }

    // Get the global_fd
    int global_fd = current_pcb->open_fds[fd];
    // Only decrement ref_count if file is a file
    if (fd_table[global_fd].fd_type == FD_FILE) {
        // Decrement ref_count, if 0, remove from global table
        fd_table[global_fd].ref_count -= 1;
        if (fd_table[global_fd].ref_count == 0) {
            memset(&fd_table[global_fd], 0, sizeof(fd_table[global_fd]));
        }
    }

    // Remove from pcb
    current_pcb->open_fds[fd] = -1;
    current_pcb->num_open_fds -= 1;

    return 0;
}

int f_unlink(const char* fname){
    // Check if file exists
    directory_entry dir_entry;
    int position;
    position = find_file(fname, &dir_entry);
    if (position == -1) { // Error if file not found
        p_perror("File not found", FileNotFoundError);
        return -1;
    }

    // Remove file from global table
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (strcmp(fname, fd_table[i].dir_entry.name) == 0) {
            p_perror("File is open", FileIsOpenError);
            return -1;
        }
    }

    // Remove file from fs
    rm(fname);
    return 0;
}

int f_read(int fd, int n, char *buf) {
    if (fd > MAX_OPEN_FILES || (current_pcb->open_fds[fd] == -1) || fd < 0) {
        p_perror("Invalid file descriptor", InvalidFileDescriptorError);
        return -1;
    }

    // Get the global_fd, error check if its uninit or stdout
    int global_fd = current_pcb->open_fds[fd];
    if (fd_table[global_fd].fd_type != FD_FILE && fd_table[global_fd].fd_type != FD_STDIN) {
        p_perror("Invalid file descriptor", InvalidFileDescriptorError);
        return -1;
    }

    if (fd_table[global_fd].fd_type == FD_STDIN) {
        int read_bytes = read(STDIN_FILENO, buf, n);
        if (read_bytes == -1) {
            p_perror("Error reading from stdin", FileWriteError);
            return -1;
        }
        return read_bytes;
    } else { // Reading from fs file
        int actual_offset = fd_table[global_fd].offset;
        int original_offset = actual_offset;
        int fat_value = fd_table[global_fd].dir_entry.firstBlock;
        if (fat_value == 0xFFFF) {
            return 0;
        }

        // Get the the block number and offset within the block
        while (actual_offset > block_size && fat_value != 0xFFFF) {
            fat_value = fat[fat_value];
            actual_offset -= block_size;
        }
        if (actual_offset >= block_size || actual_offset > fd_table[global_fd].dir_entry.size) {
            return 0;
        }

        // Seek to correct position in file
        int read_offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1) + actual_offset, SEEK_SET);
        if (read_offset == -1) {
            p_perror("Error seeking position", FileNotFoundError);
            return -1;
        }

        int total_bytes_to_read;
        if (original_offset + n < fd_table[global_fd].dir_entry.size) {
            total_bytes_to_read = n;
        } else {
            total_bytes_to_read = fd_table[global_fd].dir_entry.size - original_offset;
        }
        int total_bytes_read = 0;
        
        while (total_bytes_to_read > 0) {
            int bytes_to_read = total_bytes_to_read;
            if (actual_offset + bytes_to_read > block_size) {
                bytes_to_read = block_size - actual_offset;
                actual_offset = 0;
            }

            int read_bytes = read(fs_fd, buf + total_bytes_read, bytes_to_read);
            if (read_bytes != bytes_to_read) {
                p_perror("Error reading from file", FileReadError);
                return -1;
            }
            total_bytes_read += read_bytes;
            fd_table[global_fd].offset += read_bytes;
            total_bytes_to_read -= read_bytes;

            // Go to next block
            fat_value = fat[fat_value];
            if (fat_value == 0xFFFF) {
                return total_bytes_read;
            }

            read_offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
            if (read_offset == -1) {
                p_perror("Error seeking position", FileNotFoundError);
                return -1;
            }
        }
        return total_bytes_read;
    }
}

int f_write(int fd, const char *str, int n) {
    // Check for valid file descriptor
    if (fd > MAX_OPEN_FILES || (current_pcb->open_fds[fd] == -1) || fd < 0) {
        p_perror("Invalid file descriptor", InvalidFileDescriptorError);
        return -1;
    }

    // Get the global_fd, error check if its uninit or stdin
    int global_fd = current_pcb->open_fds[fd];
    if (fd_table[global_fd].fd_type != FD_FILE && fd_table[global_fd].fd_type != FD_STDOUT) {
        p_perror("Improper file to write to: ", InvalidFileDescriptorError);
        return -1;
    }

    // Write to stdout if fd is stdout
    if (fd_table[global_fd].fd_type == FD_STDOUT) {
        int write_bytes = write(STDOUT_FILENO, str, n);
        if (write_bytes == -1) {
            p_perror("Error writing to stdout", FileWriteError);
            return -1;
        }
        return write_bytes;
    }

    // Check if file has write permissions
    if (fd_table[global_fd].mode == F_READ || fd_table[global_fd].mode == 0) {
        p_perror("Permission denied", PermissionError);
        return -1;
    }

    if (n < 1) {
        return 0;
    }

    int actual_offset = fd_table[global_fd].offset;
    int original_offset = actual_offset;
    int fat_value = fd_table[global_fd].dir_entry.firstBlock;
    if (actual_offset > fd_table[global_fd].dir_entry.size) {
        p_perror("Error writing to file, offset > file size", FileWriteError);
        return -1;
    }

    // Traverse through FAT by offset
    while (actual_offset > block_size && fat_value != 0xFFFF) {
        fat_value = fat[fat_value];
        actual_offset -= block_size;
    }

    int total_bytes_to_write = n;
    int total_bytes_written = 0;
    int prev_fat_value = fat_value;
    if (fat_value == 0xFFFF) {
        fat_value = find_fat();
        fat[fat_value] = 0xFFFF;
        fd_table[global_fd].dir_entry.firstBlock = fat_value;
    }

    while (total_bytes_to_write > 0) {
        int bytes_to_write = total_bytes_to_write;
        if (actual_offset + bytes_to_write > block_size) {
            bytes_to_write = block_size - actual_offset;
        }

        // If new block is needed, find a new block
        if (fat_value == 0xFFFF) {
            int next_fat_value = find_fat();
            if (next_fat_value == -1) {
                p_perror("No more space left", NoMoreSpaceError);
                break;
            }
            fat_value = next_fat_value;
            if (prev_fat_value != 0xFFFF) {
                fat[prev_fat_value] = fat_value;
            }
            fat[fat_value] = 0xFFFF;
        }

        int write_offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1) + actual_offset, SEEK_SET);
        if (write_offset == -1) {
            p_perror("Error seeking position", FileNotFoundError);
            return -1;
        }

        int write_bytes = write(fs_fd, str, bytes_to_write);
        if (write_bytes != bytes_to_write) {
            p_perror("Error writing to file", FileWriteError);
            return -1;
        }
        // fd_table[global_fd].offset += write_bytes;
        total_bytes_to_write -= write_bytes;
        total_bytes_written += write_bytes;
        fat_value = fat[fat_value];
        actual_offset = 0;
    }

    // Update file size and offset
    fd_table[global_fd].offset += total_bytes_written;
    if (fd_table[global_fd].offset > fd_table[global_fd].dir_entry.size) {
        fd_table[global_fd].dir_entry.size = fd_table[global_fd].offset;
    }
    directory_entry temp_dir_entry;
    int dir_position = find_file(fd_table[global_fd].dir_entry.name, &temp_dir_entry);
    update_fs_dir_entry(fd_table[global_fd].dir_entry, dir_position);

    return total_bytes_written;
}

int f_lseek(int fd, int offset, int whence) {
    if (fd > MAX_OPEN_FILES || (current_pcb->open_fds[fd] == -1) || fd < 0) {
        p_perror("Invalid file descriptor", InvalidFileDescriptorError);
        return -1;
    }

    // Get the global_fd, error check if its uninit or stdin
    int global_fd = current_pcb->open_fds[fd];
    if (fd_table[global_fd].fd_type != FD_FILE) {
        p_perror("Invalid file descriptor", InvalidFileDescriptorError);
        return -1;
    }

    switch (whence) {
        case F_SEEK_SET:
            fd_table[global_fd].offset = offset;
            break;
        case F_SEEK_CUR:
            fd_table[global_fd].offset += offset;
            break;
        case F_SEEK_END:
            fd_table[global_fd].offset = fd_table[global_fd].dir_entry.size + offset;
            break;
        default:
            return -1;
    }
    return fd_table[global_fd].offset;
}

int f_mount(const char *fs_name) {
    if (mount(fs_name) == -1) {
        return -1;
    }

    memset(fd_table, 0, sizeof(fd_table));
    fd_table[0].fd_type = FD_STDIN;
    fd_table[0].mode = F_READ;
    fd_table[1].fd_type = FD_STDOUT;
    fd_table[1].mode = F_WRITE;
    return 0;
}

int f_touch(struct parsed_command *cmd) {
    return touch(cmd);
}

int f_rm(const char *fs_name) {
    return rm(fs_name);
}

int f_mv(const char *src, const char *dst) {
    return mv(src, dst);
}

int f_cp(struct parsed_command *cmd) {
    return cp(cmd);
}

int f_cat(struct parsed_command *cmd) {
    int length = 1;
    if (strcmp(cmd->commands[0][1], "-w") == 0) {
        return cat_w_f(cmd);
    } else if (strcmp(cmd->commands[0][1], "-a") == 0) {
        return cat_a_f(cmd);
    }

    // get the last argument
    while (cmd->commands[0][length] != NULL) {
        length++;
    }
    
    if (strcmp(cmd->commands[0][length - 2], "-w") == 0) { //output file
        return cat_f_w(cmd);
    } else if (strcmp(cmd->commands[0][length - 2], "-a") == 0) { //output file
        return cat_f_a(cmd);
    } else {
        return cat_f(cmd);
    }
}

int f_ls() {
    return ls();
}

int f_chmod(const char* mode, const char* fs_name) {
    return chmod(mode, fs_name);
}

int f_seek(FILE *stream, long int offset, int whence) {
    return fseek(stream, offset, whence);
}

char* f_gets(char *str, int n, FILE *stream) {
    return fgets(str, n, stream);
}

int f_find_file(const char *fname, directory_entry *result) {
    return find_file(fname, result);
}

char* f_strtok(char *str, const char *delim) {
    return strtok(str, delim);
}

int f_fprintf(FILE *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);

    int result = vfprintf(stream, format, args);

    va_end(args);
    return result;
}