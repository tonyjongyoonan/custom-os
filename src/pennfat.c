#include "pennfat.h"
#include "f_pennos.h"

#define MAX_LINE_LENGTH 4096

int fs_fd = -1;
uint16_t *fat = NULL; // Pointer to the FAT in memory
size_t fat_size = 0; // Size of the currently mounted FAT
int block_size = 0; // Size of a block in the currently mounted FAT
//extern FileDescriptor fd_table[MAX_OPEN_FILES];


int find_file(const char* fname, directory_entry *result) {
    directory_entry dir_entry;
    int fat_value = 1;
    size_t num_entries = block_size / sizeof(directory_entry);

    // Traverse through the root directory
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "Error seeking\n");
            return -1;
        }
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // Check if the current entry is the source file
            if (strcmp(dir_entry.name, fname) == 0) {
                off_t current_pos = lseek(fs_fd, 0, SEEK_CUR);
                *result = dir_entry;
                return current_pos - sizeof(directory_entry);
            }
        }
        fat_value = fat[fat_value];
    }
    return -1;
}

// Helper to initialize the FAT area in the file system
int initialize_fat(int fs_fd, int blocks_in_fat, int block_size_config) {   
    uint16_t *fat_buffer = calloc(2, sizeof(uint16_t)); // FAT[0] and FAT[1]
    if (fat_buffer == NULL) {
        fprintf(stderr, "Failed to allocate FAT buffer\n");
        return -1;
    }
    
    // Metadata for the FAT
    fat_buffer[0] = (blocks_in_fat << 8) | block_size_config;
    // Initial root directory block
    fat_buffer[1] = 0xFFFF;
    
    // Seek to the beginning of the file to replace the 0s with the FAT metadata
    if (lseek(fs_fd, 0, SEEK_SET) == (off_t)-1) {
        fprintf(stderr, "Failed to seek to beginning of file\n");
        free(fat_buffer);
        return -1;
    }
    
    // Write the FAT metadata to the file
    ssize_t written = write(fs_fd, fat_buffer, 2 * sizeof(uint16_t));
    free(fat_buffer);
    if (written != 2 * sizeof(uint16_t)) {
        fprintf(stderr, "Failed to write FAT metadata to file system file\n");
        return -1;
    }

    return 0;
}

/*
Creates a PennFAT filesystem in the file named FS_NAME. 
The number of blocks in the FAT region is BLOCKS_IN_FAT (ranging from 1 through 32), 
and the block size is 512, 1024, 2048, or 4096 bytes corresponding 
to the value (1 through 4) of BLOCK_SIZE_CONFIG.*/
int mkfs(const char *fs_name, int blocks_in_fat, int block_size_config) {
    // Error checking for blocks_in_fat and block_size_config
    if (blocks_in_fat < 1 || blocks_in_fat > 32) {
        fprintf(stderr, "Invalid value for blocks_in_fat. It should be between 1 and 32.\n");
        return -1;
    }
    if (block_size_config < 0 || block_size_config > 4) {
        fprintf(stderr, "Invalid value for block_size_config. It should be between 0 and 4.\n");
        return -1;
    }

    // Information for sizes and num entries
    int block_size = 1 << (block_size_config + 8); // Q? error-checking for block_size_config
    int num_fat_entries = block_size * blocks_in_fat / 2;
    if (num_fat_entries > 0xFFFF) {
        num_fat_entries = 0xFFFF;
    }
    int fat_size = block_size * blocks_in_fat; 
    size_t data_region_size = block_size * (num_fat_entries - 1);
    size_t total_file_size = fat_size + data_region_size;

    // Open the file system file, create if it does not exist
    int fs_fd = open(fs_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fs_fd == -1) {
        fprintf(stderr, "Failed to open file system file\n");
        return -1;
    }

    // Zero out the entire file
    if (ftruncate(fs_fd, total_file_size) == -1) {
        fprintf(stderr, "Failed to set file system size\n");
        close(fs_fd);
        return -1;
    }
    
    // Initialize the FAT metadata + root directory in the file
    if (initialize_fat(fs_fd, blocks_in_fat, block_size_config) != 0) {
        close(fs_fd);
        return -1;
    }

    close(fs_fd);
    return 0;
}

// Helper to get fat size from metadata
size_t get_fat_size_from_metadata(uint16_t metadata) {
    uint16_t blocks_in_fat = metadata >> 8;
    uint16_t block_size_config = metadata & 0xFF;

    block_size = 1 << (block_size_config + 8);
    size_t fat_size = block_size * blocks_in_fat;

    return fat_size;
}

// Mounts the file system specified at fs_name
int mount(const char *fs_name) {
    if (fs_fd != -1) {
        fprintf(stderr, "A filesystem is already mounted.\n");
        return -1;
    }

    // Open the file system file
    fs_fd = open(fs_name, O_RDWR);
    if (fs_fd == -1) {
        fprintf(stderr, "Failed to open file system file\n");
        return -1;
    }

    // Read metadata
    uint16_t metadata;
    if (read(fs_fd, &metadata, sizeof(metadata)) != sizeof(metadata)) {
        fprintf(stderr, "Failed to read FAT metadata\n");
        close(fs_fd);
        return -1;
    }

    // Calculate fat_size from metadata (fat[0])
    fat_size = get_fat_size_from_metadata(metadata);

    // Use mmap to map the FAT region into memory
    fat = mmap(NULL, fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    if (fat == MAP_FAILED) {
        fprintf(stderr, "Failed to map FAT into memory\n");
        close(fs_fd);
        fs_fd = -1;
        fat_size = 0;
        block_size = 0;
        return -1;
    }
    return 0;
}

// Unmounts current file system FAT
int umount() {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted.\n");
        return -1;
    }

    // Unmap the FAT from memory
    if (munmap(fat, fat_size) == -1) {
        fprintf(stderr, "Failed to unmap FAT from memory\n");
        return -1;
    }

    // Close the file system file
    close(fs_fd);
    fs_fd = -1;
    fat = NULL;
    fat_size = 0;
    block_size = 0;
    return 0;
}

int find_new_fat() {
    int fat_no = 2;
    char fat_entry[3];
    fat_entry[2] = '\0';

    int offset = lseek(fs_fd, fat_no * 2, SEEK_SET);
    if (offset == -1) {
        fprintf(stderr, "No more space left\n");
        return -1;
    }

    int read_bytes = read(fs_fd, fat_entry, 2);
    if (read_bytes == -1) {
        fprintf(stderr, "Error reading directory entry for destination\n");
        return -1;
    }

    while (strcmp(fat_entry, "") != 0) {
        fat_no++;
        int offset = lseek(fs_fd, fat_no * 2, SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        int read_bytes = read(fs_fd, fat_entry, 2);
        if (read_bytes == -1) {
            fprintf(stderr, "Error reading directory entry for destination\n");
            return -1;
        }
    }
    return fat_no;
}


int touch_single(const char *fs_name) {
    int fat_value = 1;
    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;

    // seek to the start of root directory
    int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
    if (root_dir_offset == -1) {
        fprintf(stderr, "Failed to seek to root directory\n");
        return -1;
    }

    // traverse to see if file exists
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // Check if the current entry is the source file
            if (strcmp(dir_entry.name, fs_name) == 0) {
                // source file already exists, update timestamp to current time
                dir_entry.mtime = time(NULL);
                return 0;
            }
        }
        fat_value = fat[fat_value];
    }

    // final block
    for (int i = 0; i < num_entries; i++) {
        int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
        if (read_bytes == -1) {
            fprintf(stderr, "Error reading directory entry\n");
            return -1;
        }

        // Check if the current entry is the source file
        if (strcmp(dir_entry.name, fs_name) == 0) {
            // source file already exists, update timestamp to current time
            dir_entry.mtime = time(NULL);
            return 0;
        }
    }
    // create new directory entry
    directory_entry new_dir_entry;
    strncpy(new_dir_entry.name, fs_name, sizeof(new_dir_entry.name));
    new_dir_entry.size = 0;
    new_dir_entry.firstBlock = 0xFFFF;
    new_dir_entry.type = 1;
    new_dir_entry.perm = 6;
    new_dir_entry.mtime = time(NULL);

    // seek to the start of root directory
    root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
    if (root_dir_offset == -1) {
        fprintf(stderr, "Failed to seek to root directory\n");
        return -1;
    }

    // navigate to next available space in root directory
    fat_value = 1;
    int final_block = fat_value;
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        // one block
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // check if current entry is empty
            if (strncmp(dir_entry.name, "", sizeof(dir_entry.name)) == 0) {
                // found empty entry, write new entry here
                int offset = lseek(fs_fd, fat_size + (fat_value - 1) * block_size + i * sizeof(directory_entry), SEEK_SET); // seek to empty entry since we overshot
                if (offset == -1) {
                    fprintf(stderr, "Failed to seek to empty entry\n");
                    return -1;
                }

                int write_bytes = write(fs_fd, &new_dir_entry, sizeof(directory_entry));
                if (write_bytes == -1) {
                    fprintf(stderr, "Error writing directory entry\n");
                    return -1;
                }
                return 0;
            }
        }

        if (fat[fat_value] == 0xFFFF) {
            final_block = fat_value;
        } // save last block of root directory
        fat_value = fat[fat_value];
    }

    // if we reach here, there is no more space in current block--find new block
    int new_fat = find_new_fat();
    fat[final_block] = new_fat;
    fat[new_fat] = 0xFFFF;

    int offset = lseek(fs_fd, fat_size + block_size * (new_fat - 1), SEEK_SET);
    if (offset == -1) {
        fprintf(stderr, "No more space left\n");
        return -1;
    }
    
    int write_bytes = write(fs_fd, &new_dir_entry, sizeof(directory_entry));
    if (write_bytes == -1) {
        fprintf(stderr, "Error writing directory entry\n");
        return -1;
    }
    
    return 0;
}

int touch(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    int length = 1;
    while (cmd->commands[0][length] != NULL) {
        if (touch_single(cmd->commands[0][length]) != 0) {
            return -1;
        }
        length++;
    }

    return 0;
}

int rm(const char *fs_name) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    directory_entry dir_entry;
    off_t current_pos = find_file(fs_name, &dir_entry);
    if (current_pos == -1) {
        fprintf(stderr, "File not found\n");
        return -1;
    }

    // Delete the destination FAT chain in the FAT
    uint16_t fat_value = dir_entry.firstBlock;
    while (fat_value != 0xFFFF) {
        lseek(fs_fd, fat_size + ((fat_value - 1) * block_size), SEEK_SET); // Seek to the block to delete
        char zero_block[block_size];
        memset(zero_block, 0, block_size);
        write(fs_fd, zero_block, block_size); // Zero out the block

        // Update FAT entries
        uint16_t next_fat_value = fat[fat_value];
        fat[fat_value] = 0;
        fat_value = next_fat_value;
    }

    // Zero our root directory entry
    directory_entry dir_entry_zero;
    lseek(fs_fd, current_pos, SEEK_SET); // Go back to the file entry
    memset(&dir_entry_zero, 0, sizeof(directory_entry)); // Zero out entry
    ssize_t bytes_written = write(fs_fd, &dir_entry_zero, sizeof(directory_entry));
    if (bytes_written == -1) {
        fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int find_open_fat() {
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


int mv(const char *src, const char *dst) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;
    int root_fat_value = 1;

    while(root_fat_value != 0xFFFF) {
        off_t root_dir_offset = lseek(fs_fd, fat_size + ((root_fat_value - 1) * block_size), SEEK_SET);
        if (root_dir_offset == (off_t)-1) {
            fprintf(stderr, "Failed to seek to root directory\n");
            return -1;
        }

        for (size_t i = 0; i < num_entries; i++) {
            ssize_t read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // Search for source file
            if (strncmp(dir_entry.name, src, sizeof(dir_entry.name)) == 0) {
                
                // Save position of source
                off_t current_pos = lseek(fs_fd, 0, SEEK_CUR);

                int root_fat_value_dst = 1;
                directory_entry dst_entry;
                int found_dst = 0;

                while(root_fat_value_dst != 0xFFFF && !found_dst) {
                    root_dir_offset = lseek(fs_fd, fat_size + ((root_fat_value_dst - 1) * block_size), SEEK_SET);

                    // Search for an existing destination file
                    for (size_t j = 0; j < num_entries; j++) {
                        ssize_t read_bytes_dst = read(fs_fd, &dst_entry, sizeof(directory_entry));
                        if (read_bytes_dst == -1) {
                            fprintf(stderr, "Error reading directory entry for destination\n");
                            return -1;
                        }

                        // Remove destination file if it exists
                        if (strncmp(dst_entry.name, dst, sizeof(dst_entry.name)) == 0) {
                            rm(dst);
                            found_dst = 1;
                            break;
                        }
                    }
                    root_fat_value_dst = fat[root_fat_value_dst];
                }

                // Rename the source file to the destination
                lseek(fs_fd, current_pos - sizeof(directory_entry), SEEK_SET); // Go back to the source file entry
                strncpy(dir_entry.name, dst, sizeof(dir_entry.name));
                dir_entry.name[sizeof(dir_entry.name) - 1] = '\0';
                dir_entry.mtime = time(NULL);
                write(fs_fd, &dir_entry, sizeof(directory_entry)); // Write back updated entry back
                return 0;
            }
        }
        root_fat_value = fat[root_fat_value];
    }

    fprintf(stderr, "Source file not found\n");
    return -1;
}

int cp(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    const char *src = NULL, *dst = NULL;
    int host_src = -1, host_dst = -1;

    if (cmd->commands[0][3] != NULL) {
        if (cmd->commands[0][1] != NULL && strcmp(cmd->commands[0][1], "-h") == 0) {
            src = cmd->commands[0][2], dst = cmd->commands[0][3], host_src = 1, host_dst = 0;
        } else if (cmd->commands[0][2] != NULL && strcmp(cmd->commands[0][2], "-h") == 0) {
            src = cmd->commands[0][1], dst = cmd->commands[0][3], host_src = 0, host_dst = 1;
        }
    } else {
        if (cmd->commands[0][1] != NULL && cmd->commands[0][2] != NULL) {
            src = cmd->commands[0][1], dst = cmd->commands[0][2], host_src = 0, host_dst = 0;
        }
    }

    if (src == NULL || dst == NULL || host_src == -1 || host_dst == -1) {
        fprintf(stderr, "Error: cp usage.\n");
        return -1;
    }

    if (host_src && !host_dst) {
        int src_fd = open(src, O_RDONLY);
        if (src_fd == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "Source file does not exist\n");
            } else {
                fprintf(stderr, "Error opening file\n");
            }
            return -1;
        }

        directory_entry dir_entry;
        if (find_file(dst, &dir_entry) == -1) {
            // Create the destination file (if it doesn't exist)
            touch_single(dst);
        } else { // Remove the destination file (if it exists)
            rm(dst);
            touch_single(dst);
        }

        int current_pos = find_file(dst, &dir_entry);

        uint16_t fat_value = find_open_fat();
        uint16_t prev_fat_value = 0xFFFF;
        (dir_entry).firstBlock = fat_value;
        fat[fat_value] = 0xFFFF;
        char buffer[block_size];
        ssize_t bytes_read, bytes_written;
        uint32_t total_written = 0;

        while ((bytes_read = read(src_fd, buffer, block_size)) > 0) {
            if (fat_value == 0xFFFF) {
                // Allocate a new block
                int open_fat_value = find_open_fat();
                if (open_fat_value == -1) {
                    fprintf(stderr, "No more space in FAT\n");
                    close(src_fd);
                    return -1;
                }
                fat_value = open_fat_value;
                if (prev_fat_value != 0xFFFF) {
                    fat[prev_fat_value] = fat_value;
                }
                fat[fat_value] = 0xFFFF;
            }

            off_t write_position = lseek(fs_fd, fat_size + ((fat_value - 1) * block_size), SEEK_SET);
            bytes_written = write(fs_fd, buffer, bytes_read);
            if (bytes_written == -1) {
                fprintf(stderr, "Error writing to destination file\n");
                close(src_fd);
                return -1;
            }
            total_written += bytes_written;
            prev_fat_value = fat_value;
            fat_value = fat[fat_value];
        }

        lseek(fs_fd, current_pos, SEEK_SET);
        dir_entry.mtime = time(NULL);
        dir_entry.size = total_written;
        write(fs_fd, &dir_entry, sizeof(directory_entry));
        close(src_fd);
        return 0;
    } else if (!host_src && host_dst) {
        int dst_fd = open(dst, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (dst_fd == -1) {
            fprintf(stderr, "Error opening destination file\n");
            return -1;
        }

        directory_entry dir_entry;
        int current_pos = find_file(src, &dir_entry);
        if (current_pos == -1) {
            fprintf(stderr, "Source file not found\n");
            return -1;
        }

        uint16_t fat_value = dir_entry.firstBlock;
        uint32_t size_to_read = dir_entry.size;
        char buffer[block_size];

        while (fat_value != 0xFFFF) {
            int buffer_read_size = size_to_read > block_size ? block_size : size_to_read;
            off_t block_to_read = lseek(fs_fd, fat_size + ((fat_value - 1) * block_size), SEEK_SET);
            ssize_t bytes_read = read(fs_fd, buffer, buffer_read_size);
            if (bytes_read == -1) {
                fprintf(stderr, "Error reading from source\n");
                close(dst_fd);
                return -1;
            }
            ssize_t bytes_written = write(dst_fd, buffer, bytes_read);
            if (bytes_written == -1) {
                fprintf(stderr, "Error writing to destination file\n");
                close(dst_fd);
                return -1;
            }

            uint16_t next_fat_value = fat[fat_value];
            size_to_read -= bytes_read;
            if (size_to_read <= 0) {
                break;
            }
            fat_value = next_fat_value;
        }

        close(dst_fd);
        return 0;
    } else if (!host_src && !host_dst) {
        directory_entry src_dir_entry;
        directory_entry dst_dir_entry;

        if (find_file(src, &src_dir_entry) == -1) {
            fprintf(stderr, "Source file not found\n");
            return -1;
        }

        if (find_file(dst, &dst_dir_entry) == -1) {
            // Create the destination file (if it doesn't exist)
            touch_single(dst);
        } else { // Remove the destination file (if it exists)
            rm(dst);
            touch_single(dst);
        }

        // Find position and directory entry of source and dest
        int current_src_pos = find_file(src, &src_dir_entry);
        if (current_src_pos == -1) {
            fprintf(stderr, "Source file not found\n");
            return -1;
        }
        int current_dst_pos = find_file(dst, &dst_dir_entry);

        uint16_t src_fat_value = src_dir_entry.firstBlock;
        uint32_t size_to_read = src_dir_entry.size;
        uint16_t dst_fat_value = find_open_fat();
        uint16_t prev_dst_fat_value = dst_fat_value;
        dst_dir_entry.firstBlock = dst_fat_value;
        fat[dst_fat_value] = 0xFFFF;
        char buffer[block_size];
        uint32_t total_written = 0;

        while (src_fat_value != 0xFFFF) {
            // Seek to source block and read it
            int buffer_read_size = size_to_read > block_size ? block_size : size_to_read;
            off_t block_to_read = lseek(fs_fd, fat_size + ((src_fat_value - 1) * block_size), SEEK_SET);
            ssize_t bytes_read = read(fs_fd, buffer, buffer_read_size);

            // Allocate new block for destination if needed
            if (dst_fat_value == 0xFFFF) {
                int open_fat_value = find_open_fat();
                if (open_fat_value == -1) {
                    fprintf(stderr, "No more space in FAT\n");
                    return -1;
                }
                dst_fat_value = open_fat_value;
                fat[prev_dst_fat_value] = dst_fat_value;
                fat[dst_fat_value] = 0xFFFF;
            }

            // Seek to destination block and write it
            off_t block_to_write = lseek(fs_fd, fat_size + ((dst_fat_value - 1) * block_size), SEEK_SET);
            ssize_t bytes_written = write(fs_fd, buffer, bytes_read);
            total_written += bytes_written;
            
            // Update next blocks to read/write for src
            src_fat_value = fat[src_fat_value];
            size_to_read -= bytes_read;
            if (size_to_read <= 0) {
                break;
            }

            // Update next dst block to write to
            prev_dst_fat_value = dst_fat_value;
            dst_fat_value = fat[dst_fat_value];
        }

        lseek(fs_fd, current_dst_pos, SEEK_SET);
        dst_dir_entry.mtime = time(NULL);
        dst_dir_entry.size = total_written;
        write(fs_fd, &dst_dir_entry, sizeof(directory_entry));
    }
    return 0;
}

// cat FILE ... [ -w OUTPUT_FILE ]
// Concatenates the files and overwrites OUTPUT_FILE. 
// If OUTPUT_FILE does not exist, it will be created. (Same for OUTPUT_FILE in the commands below.)
int cat_f_w(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    size_t data_region_size = block_size * (fat_size / 2 - 1);
    char input[fat_size + data_region_size];
    input[0] = '\0';
    int length = 1;
    int total_bytes_written = 0;
    while (strcmp(cmd->commands[0][length], "-w") != 0) {
        // find file in root directory
        int fat_value = 1;
        size_t num_entries = block_size / sizeof(directory_entry);
        directory_entry dir_entry;

        int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
        if (root_dir_offset == -1) {
            fprintf(stderr, "Failed to seek to root directory\n");
            return -1;
        }
        // find the file in root directory (directory may be multiple blocks)
        while (fat_value != 0xFFFF) {
            int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
            if (offset == -1) {
                fprintf(stderr, "No more space left\n");
                return -1;
            }
            for (int i = 0; i < num_entries; i++) {
                int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
                if (read_bytes == -1) {
                    fprintf(stderr, "Error reading directory entry\n");
                    return -1;
                }

                // found the file in root directory
                if (strncmp(dir_entry.name, cmd->commands[0][length], sizeof(dir_entry.name)) == 0) {
                    total_bytes_written += dir_entry.size;
                    int new_fat_value = dir_entry.firstBlock; // find first block of file
                    if (dir_entry.firstBlock == 0xFFFF) {
                        break; // nothing written to file yet
                    }

                    // traverse through blocks of file
                    while (new_fat_value != 0xFFFF) {
                        char block[block_size];
                        int offset = lseek(fs_fd, fat_size + block_size * (new_fat_value - 1), SEEK_SET);
                        if (offset == -1) {
                            fprintf(stderr, "No more space left\n");
                            return -1;
                        }

                        int read_bytes = read(fs_fd, block, block_size);
                        if (read_bytes == -1) {
                            fprintf(stderr, "Error reading directory entry for destination\n");
                            return -1;
                        }

                        strcat(input, block);
                        new_fat_value = fat[new_fat_value];
                    }
                }
            }
            if (dir_entry.firstBlock == 0xFFFF) {
                break;
            }
            fat_value = fat[fat_value];
        }    
        length++; 
    }
    
    
    // find file in root directory
    int fat_value = 1;
    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;

    int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
    if (root_dir_offset == -1) {
        fprintf(stderr, "Failed to seek to root directory\n");
        return -1;
    }
    // traverse through entire root directory (may be multiple blocks)
    bool file_found = false;
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // found the file in root directory
            if (strncmp(dir_entry.name, cmd->commands[0][length + 1], sizeof(dir_entry.name)) == 0) {   
                file_found = true;
                int directory_pos = lseek(fs_fd, -sizeof(directory_entry), SEEK_CUR);
                if (directory_pos == -1) {
                    fprintf(stderr, "Failed to seek to root directory\n");
                    return -1;
                }
                int curr_fat_block = dir_entry.firstBlock;
                if (curr_fat_block == 0xFFFF) {
                    curr_fat_block = find_new_fat();
                    fat[curr_fat_block] = 0xFFFF;
                    dir_entry.firstBlock = curr_fat_block;
                    
                    // update directory
                    directory_pos = lseek(fs_fd, directory_pos, SEEK_SET); // seek back to entry
                    if (directory_pos == -1) {
                        fprintf(stderr, "Failed to seek to root directory\n");
                        return -1;
                    }
                    int write_bytes = write(fs_fd, &dir_entry, sizeof(directory_entry));
                }
                int next_fat_block = fat[curr_fat_block];

                // get input
                int i = 0;
                bool nullify = false;
                while (input[i] != '\0' && input[i] != '\n') {
                    char block[block_size];
                    for (int j = 0; j < block_size / sizeof(char) - 1; j++) {
                        if (input[i] == '\0' || input[i] == '\n') {
                            block[j] = '\0';
                            nullify = true;
                            i++;
                        } else if (nullify) {
                            block[j] = '\0';
                            i++;
                        } else {
                            block[j] = input[i];
                            i++;
                        }
                    }

                    // write to curr_fat_block
                    int offset = lseek(fs_fd, fat_size + block_size * (curr_fat_block - 1), SEEK_SET);
                    if (offset == -1) {
                        fprintf(stderr, "No more space left\n");
                        return -1;
                    }
                    int write_bytes = write(fs_fd, block, block_size);
                    if (write_bytes == -1) {
                        fprintf(stderr, "Error writing directory entry\n");
                        return -1;
                    }

                    // update FAT
                    if (next_fat_block == 0xFFFF) { // no next block
                        next_fat_block = find_new_fat();
                        fat[curr_fat_block] = next_fat_block;
                        fat[next_fat_block] = 0xFFFF;
                        curr_fat_block = next_fat_block;
                        next_fat_block = 0xFFFF;
                    } else { // available next block
                        fat[curr_fat_block] = next_fat_block;
                        curr_fat_block = next_fat_block;
                        next_fat_block = fat[curr_fat_block];
                    }
                }
            }
        }
        fat_value = fat[fat_value];
    }

    if (file_found) {
        return 0;
    } else {
        // not found in directory, create and write
        touch_single(cmd->commands[0][length + 1]);

        // find file we just created in root directory
        int fat_value = 1;
        size_t num_entries = block_size / sizeof(directory_entry);
        directory_entry dir_entry;

        int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
        if (root_dir_offset == -1) {
            fprintf(stderr, "Failed to seek to root directory\n");
            return -1;
        }
        // traverse through entire root directory (may be multiple blocks)
        while (fat_value != 0xFFFF) {
            int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
            if (offset == -1) {
                fprintf(stderr, "No more space left\n");
                return -1;
            }
            for (int i = 0; i < num_entries; i++) {
                int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
                if (read_bytes == -1) {
                    fprintf(stderr, "Error reading directory entry\n");
                    return -1;
                }
                // found the file in root directory
                if (strncmp(dir_entry.name, cmd->commands[0][length + 1], sizeof(dir_entry.name)) == 0) {
                    int directory_pos = lseek(fs_fd, -sizeof(directory_entry), SEEK_CUR);
                    if (directory_pos == -1) {
                        fprintf(stderr, "Failed to seek to root directory\n");
                        return -1;
                    }
                    int curr_fat_block = dir_entry.firstBlock;
                    if (curr_fat_block == 0xFFFF) {
                        curr_fat_block = find_new_fat();
                        fat[curr_fat_block] = 0xFFFF;
                        dir_entry.firstBlock = curr_fat_block;
                    }
                    dir_entry.mtime = time(NULL);
                    dir_entry.size = total_bytes_written;
                        
                    // update directory
                    directory_pos = lseek(fs_fd, directory_pos, SEEK_SET); // seek back to entry
                    if (directory_pos == -1) {
                        fprintf(stderr, "Failed to seek to root directory\n");
                        return -1;
                    }
                    int write_bytes = write(fs_fd, &dir_entry, sizeof(directory_entry));
                    if (write_bytes == -1) {
                        fprintf(stderr, "Error writing directory entry\n");
                        return -1;
                    }

                    int next_fat_block = fat[curr_fat_block];

                    // get input
                    int i = 0;
                    bool nullify = false;
                    while (input[i] != '\0' && input[i] != '\n') {
                        char block[block_size];
                        for (int j = 0; j < block_size / sizeof(char) - 1; j++) {
                            if (input[i] == '\0' || input[i] == '\n') {
                                block[j] = '\0';
                                nullify = true;
                                i++;
                            } else if (nullify) {
                                block[j] = '\0';
                                i++;
                            } else {
                                block[j] = input[i];
                                i++;
                            }
                        }

                        // write to curr_fat_block
                        int offset = lseek(fs_fd, fat_size + block_size * (curr_fat_block - 1), SEEK_SET);
                        if (offset == -1) {
                            fprintf(stderr, "No more space left\n");
                            return -1;
                        }
                        int write_bytes = write(fs_fd, block, block_size);
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }

                        // update FAT
                        if (next_fat_block == 0xFFFF) { // no next block
                            next_fat_block = find_new_fat();
                            fat[curr_fat_block] = next_fat_block;
                            fat[next_fat_block] = 0xFFFF;
                            curr_fat_block = next_fat_block;
                            next_fat_block = 0xFFFF;
                        } else { // available next block
                            fat[curr_fat_block] = next_fat_block;
                            curr_fat_block = next_fat_block;
                            next_fat_block = fat[curr_fat_block];
                        }
                    }
                }
            }
            fat_value = fat[fat_value];
        }
    }
    return 0;
}

// cat FILE ... -a OUTPUT_FILE
// Concatenates the files and appends to OUTPUT_FILE.
int cat_f_a(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    size_t data_region_size = block_size * (fat_size / 2 - 1);
    char str[data_region_size]; // malloc instead
    str[0] = '\0';
    int length = 1;
    int total_bytes_written = 0;
    while (strcmp(cmd->commands[0][length], "-a") != 0) {
        // find file in root directory
        int fat_value = 1;
        size_t num_entries = block_size / sizeof(directory_entry);
        directory_entry dir_entry;

        int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
        if (root_dir_offset == -1) {
            fprintf(stderr, "Failed to seek to root directory\n");
            return -1;
        }
        // find the file in root directory (directory may be multiple blocks)
        while (fat_value != 0xFFFF) {
            int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
            if (offset == -1) {
                fprintf(stderr, "No more space left\n");
                return -1;
            }
            for (int i = 0; i < num_entries; i++) {
                int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
                if (read_bytes == -1) {
                    fprintf(stderr, "Error reading directory entry\n");
                    return -1;
                }

                // found the file in root directory
                if (strncmp(dir_entry.name, cmd->commands[0][length], sizeof(dir_entry.name)) == 0) {
                    total_bytes_written += dir_entry.size;
                    int new_fat_value = dir_entry.firstBlock; // find first block of file
                    if (dir_entry.firstBlock == 0xFFFF) {
                        return 0; // nothing written to file yet
                    }

                    // traverse through blocks of file
                    while (new_fat_value != 0xFFFF) {
                        char b[block_size + 1];
                        int offset = lseek(fs_fd, fat_size + block_size * (new_fat_value - 1), SEEK_SET);
                        if (offset == -1) {
                            fprintf(stderr, "No more space left\n");
                            return -1;
                        }

                        int read_bytes = read(fs_fd, b, block_size);
                        if (read_bytes == -1) {
                            fprintf(stderr, "Error reading directory entry for destination\n");
                            return -1;
                        }

                        // null terminate block
                        b[block_size] = '\0';
                        strcat(str, b);
                        new_fat_value = fat[new_fat_value];
                    }
                }
            }
            fat_value = fat[fat_value];
        }    
        length++; 
    }

    // find output file in root directory
    int fat_value = 1;
    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;

    int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
    if (root_dir_offset == -1) {
        fprintf(stderr, "Failed to seek to root directory\n");
        return -1;
    }
    // traverse through entire root directory (may be multiple blocks)
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // found the file in root directory
            if (strncmp(dir_entry.name, cmd->commands[0][length + 1], sizeof(dir_entry.name)) == 0 && read_bytes != 0) {  
                int directory_pos = lseek(fs_fd, -sizeof(directory_entry), SEEK_CUR);
                if (directory_pos == -1) {
                    fprintf(stderr, "Failed to seek to root directory\n");
                    return -1;
                }
                int last_fat_block = dir_entry.firstBlock; // find first block of file
                if (dir_entry.firstBlock != 0xFFFF) {
                    while (fat[last_fat_block] != 0xFFFF) {
                        // navigate to end of file
                        last_fat_block = fat[last_fat_block];
                    }
                }

                // get input
                size_t len = 0;
                int i = 0;
                bool nullify = false;
                bool append = dir_entry.firstBlock != 0xFFFF;

                // print str
                while (str[i] != '\0') {
                    char block[block_size];
                    if (append) {
                        lseek(fs_fd, fat_size + block_size * (last_fat_block - 1), SEEK_SET);
                        read(fs_fd, &block, block_size);
                        int writeFrom = 0;
                        while (block[writeFrom] != '\0') {
                            writeFrom++;
                        }
                        for (int k = writeFrom; k < block_size / sizeof(char) - 1; k++) {
                            if (str[i] == '\0') {
                                block[k] = '\0';
                                nullify = true;
                                i++;
                            } else if (nullify) {
                                block[k] = '\0';
                                i++;
                            } else {
                                block[k] = str[i];
                                i++;
                            }
                        }
                        lseek(fs_fd, fat_size + block_size * (last_fat_block - 1), SEEK_SET);
                        write(fs_fd, block, block_size);

                        append = false;
                    } else {
                        for (int j = 0; j < block_size / sizeof(char) - 1; j++) {
                            if (str[i] == '\0') {
                                block[j] = '\0';
                                nullify = true;
                                i++;
                            } else if (nullify) {
                                block[j] = '\0';
                                i++;
                            } else {
                                block[j] = str[i];
                                i++;
                            }
                        }
                        int new_fat_block = find_new_fat();
                        int offset = lseek(fs_fd, fat_size + block_size * (new_fat_block - 1), SEEK_SET);
                        if (offset == -1) {
                            fprintf(stderr, "No more space left\n");
                            return -1;
                        }

                        int write_bytes = write(fs_fd, block, block_size);
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }
                        // update FAT
                        if (last_fat_block != 0xFFFF) { // not first entry
                            fat[last_fat_block] = new_fat_block;
                        } else { // first entry
                            dir_entry.firstBlock = new_fat_block;
                        }
                        dir_entry.mtime = time(NULL);   
                        dir_entry.size += total_bytes_written;
                        // update directory
                        directory_pos = lseek(fs_fd, directory_pos, SEEK_SET); // seek back to entry
                        if (directory_pos == -1) {
                            fprintf(stderr, "Failed to seek to root directory\n");
                            return -1;
                        }
                        write_bytes = write(fs_fd, &dir_entry, sizeof(directory_entry));
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }


                        fat[new_fat_block] = 0xFFFF; // end of file
                        last_fat_block = new_fat_block;
                    }
                }
            }
        }
        fat_value = fat[fat_value];
    }
    return 0;
}

// cat -a OUTPUT_FILE
// Reads from the terminal and appends to OUTPUT_FILE.
int cat_a_f(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    // find file in root directory
    int fat_value = 1;
    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;
    char input[block_size];
    int total_bytes_written = 0;

    int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
    if (root_dir_offset == -1) {
        fprintf(stderr, "Failed to seek to root directory\n");
        return -1;
    }
    // traverse through entire root directory (may be multiple blocks)
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // found the file in root directory
            if (strncmp(dir_entry.name, cmd->commands[0][2], sizeof(dir_entry.name)) == 0) {   
                int directory_pos = lseek(fs_fd, -sizeof(directory_entry), SEEK_CUR);
                if (directory_pos == -1) {
                    fprintf(stderr, "Failed to seek to root directory\n");
                    return -1;
                }
                int last_fat_block = dir_entry.firstBlock; // find first block of file
                if (dir_entry.firstBlock != 0xFFFF) {
                    while (fat[last_fat_block] != 0xFFFF) {
                        // navigate to end of file
                        last_fat_block = fat[last_fat_block];
                    }
                }

                // get input from stdin
                char input[MAX_LINE_LENGTH];
                int bytes_read = f_read(0, MAX_LINE_LENGTH, input);
                int i = 0;
                bool nullify = false;
                bool append = dir_entry.firstBlock != 0xFFFF;
                while (input[i] != '\0' && input[i] != '\n') {
                    char block[block_size];
                    if (append) {
                        int seek_bytes = lseek(fs_fd, fat_size + block_size * (last_fat_block - 1), SEEK_SET);
                        if (seek_bytes == -1) {
                            fprintf(stderr, "Failed to seek to root directory\n");
                            return -1;
                        }
                        int read_bytes = read(fs_fd, &block, block_size);
                        if (read_bytes == -1) {
                            fprintf(stderr, "Error reading directory entry\n");
                            return -1;
                        }

                        int writeFrom = 0;
                        while (block[writeFrom] != '\0') {
                            writeFrom++;
                        }
                        for (int k = writeFrom; k < block_size / sizeof(char) - 1; k++) {
                            if (input[i] == '\0') {
                                block[k] = '\0';
                                nullify = true;
                                i++;
                            } else if (nullify) {
                                block[k] = '\0';
                                i++;
                            } else {
                                block[k] = input[i];
                                i++;
                                total_bytes_written++;
                            }
                        }
                        seek_bytes = lseek(fs_fd, fat_size + block_size * (last_fat_block - 1), SEEK_SET);
                        if (seek_bytes == -1) {
                            fprintf(stderr, "Failed to seek to root directory\n");
                            return -1;
                        }
                        int write_bytes = write(fs_fd, block, block_size);
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }
                        append = false;

                        dir_entry.mtime = time(NULL); 
                        dir_entry.size += total_bytes_written;
                        total_bytes_written = 0;

                        // update directory
                        directory_pos = lseek(fs_fd, directory_pos, SEEK_SET); // seek back to entry
                        if (directory_pos == -1) {
                            fprintf(stderr, "Failed to seek to root directory\n");
                            return -1;
                        }
                        write_bytes = write(fs_fd, &dir_entry, sizeof(directory_entry));
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }
                    } else {
                        for (int j = 0; j < block_size / sizeof(char) - 1; j++) {
                            if (input[i] == '\0' || input[i] == '\n') {
                                block[j] = '\0';
                                nullify = true;
                                i++;
                            } else if (nullify) {
                                block[j] = '\0';
                                i++;
                            } else {
                                block[j] = input[i];
                                i++;
                                total_bytes_written++;
                            }
                        }
                        int new_fat_block = find_new_fat();
                        int offset = lseek(fs_fd, fat_size + block_size * (new_fat_block - 1), SEEK_SET);
                        if (offset == -1) {
                            fprintf(stderr, "No more space left\n");
                            return -1;
                        }

                        int write_bytes = write(fs_fd, block, block_size);
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }
                        // update FAT
                        if (last_fat_block != 0xFFFF) { // not first entry
                            fat[last_fat_block] = new_fat_block;
                        } else { // first entry
                            dir_entry.firstBlock = new_fat_block;   
                        }
                        dir_entry.mtime = time(NULL); 
                        dir_entry.size += total_bytes_written;
                        total_bytes_written = 0;

                        // update directory
                        directory_pos = lseek(fs_fd, directory_pos, SEEK_SET); // seek back to entry
                        if (directory_pos == -1) {
                            fprintf(stderr, "Failed to seek to root directory\n");
                            return -1;
                        }
                        write_bytes = write(fs_fd, &dir_entry, sizeof(directory_entry));
                        if (write_bytes == -1) {
                            fprintf(stderr, "Error writing directory entry\n");
                            return -1;
                        }

                        fat[new_fat_block] = 0xFFFF; // end of file
                        last_fat_block = new_fat_block;
                    }
                }
            }
        }
        fat_value = fat[fat_value];
    }
    return 0;
}

// cat -w OUTPUT_FILE
// Reads from the terminal and overwrites OUTPUT_FILE.
int cat_w_f(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    directory_entry dir_entry;
    off_t current_pos = find_file(cmd->commands[0][2], &dir_entry);
    if (current_pos == -1) {
        fprintf(stderr, "File not found\n");
        return -1;
    }

    // Delete the destination FAT chain in the FAT
    uint16_t fat_value = dir_entry.firstBlock;
    while (fat_value != 0xFFFF) {
        lseek(fs_fd, fat_size + ((fat_value - 1) * block_size), SEEK_SET); // Seek to the block to delete
        char zero_block[block_size];
        memset(zero_block, 0, block_size);
        write(fs_fd, zero_block, block_size); // Zero out the block

        // Update FAT entries
        uint16_t next_fat_value = fat[fat_value];
        fat[fat_value] = 0;
        fat_value = next_fat_value;
    }

    // Replace our root directory entry
    directory_entry dir_entry_reset;
    memcpy(dir_entry_reset.name, dir_entry.name, sizeof(dir_entry.name));
    dir_entry_reset.size = 0;
    dir_entry_reset.firstBlock = 0xFFFF;
    dir_entry_reset.type = 1;
    dir_entry_reset.perm = 6;
    dir_entry_reset.mtime = time(NULL);
    lseek(fs_fd, current_pos, SEEK_SET); // Go back to the file entry
    ssize_t bytes_written = write(fs_fd, &dir_entry_reset, sizeof(directory_entry));
    if (bytes_written == -1) {
        fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
        return -1;
    }

    cmd->commands[0][1] = "-a";
    return cat_a_f(cmd);
}

// cat FILE ...
// Concatenates the files and prints them to stdout by default
int cat_f(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }
    bool fileFound = false;
    int length = 1;
    while (cmd->commands[0][length] != NULL) {
        // find file in root directory
        int fat_value = 1;
        size_t num_entries = block_size / sizeof(directory_entry);
        directory_entry dir_entry;

        int root_dir_offset = lseek(fs_fd, fat_size, SEEK_SET);
        if (root_dir_offset == -1) {
            fprintf(stderr, "Failed to seek to root directory\n");
            return -1;
        }
        

        // find the file in root directory (directory may be multiple blocks)
        while (fat_value != 0xFFFF) {
            int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
            if (offset == -1) {
                fprintf(stderr, "No more space left\n");
                return -1;
            }
            for (int i = 0; i < num_entries; i++) {
                int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
                if (read_bytes == -1) {
                    fprintf(stderr, "Error reading directory entry\n");
                    return -1;
                }

                // found the file in root directory
                if (strncmp(dir_entry.name, cmd->commands[0][length], sizeof(dir_entry.name)) == 0) {
                    fileFound = true;
                    // print contents of the file
                    int new_fat_value = dir_entry.firstBlock; // find first block of file
                    if (dir_entry.firstBlock == 0xFFFF) {
                        break; // nothing written to file yet
                    }

                    // traverse through blocks of file
                    while (new_fat_value != 0xFFFF) {
                        char block[block_size];
                        int offset = lseek(fs_fd, fat_size + block_size * (new_fat_value - 1), SEEK_SET);
                        if (offset == -1) {
                            fprintf(stderr, "No more space left\n");
                            return -1;
                        }

                        int read_bytes = read(fs_fd, block, block_size);
                        if (read_bytes == -1) {
                            fprintf(stderr, "Error reading directory entry for destination\n");
                            return -1;
                        }
                        new_fat_value = fat[new_fat_value];
                        if (new_fat_value == 0xFFFF) {
                            f_write(STDOUT_FILENO, block, dir_entry.size % block_size);
                        } else {
                            f_write(STDOUT_FILENO, block, block_size);
                        }
                        //fprintf(stderr, "%s", block);
                    }
                }
            }
            if (dir_entry.firstBlock == 0xFFFF) {
                break;
            }
            fat_value = fat[fat_value];
        }    
        length++; 
    }
    if (!fileFound) {
        fprintf(stderr, "File not found\n");
        return -1;
    }
    fprintf(stderr, "\n");
    return 0;
}

int cat_all(struct parsed_command *cmd) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

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
    
    if (strcmp(cmd->commands[0][length - 2], "-w") == 0) {
        return cat_f_w(cmd);
    } else if (strcmp(cmd->commands[0][length - 2], "-a") == 0) {
        return cat_f_a(cmd);
    } else {
        return cat_f(cmd);
    }
}

int ls() {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    // Initialize num_entries and dir_entry buffer
    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;

    int fat_value = 1;
    while (fat_value != 0xFFFF) {
        int offset = lseek(fs_fd, fat_size + block_size * (fat_value - 1), SEEK_SET);
        if (offset == -1) {
            fprintf(stderr, "No more space left\n");
            return -1;
        }
        for (int i = 0; i < num_entries; i++) {
            int read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            if (strncmp(dir_entry.name, "", sizeof(dir_entry.name)) != 0 && read_bytes != 0) {
                // print entry: first block number, permissions, size, month, day, time, and name.
                struct tm *time_info = gmtime(&dir_entry.mtime);

                char time_str[20]; // Adjust the size as needed
                strftime(time_str, sizeof(time_str), "%b %d %H:%M", time_info);
                char perm[4];
                char perm_value[3];
                sprintf(perm_value, "%d", dir_entry.perm);

                if (strcmp(perm_value, "0") == 0) {
                    strcpy(perm, "---");
                } else if (strcmp(perm_value, "2") == 0) {
                    strcpy(perm, "-w-");
                } else if (strcmp(perm_value, "4") == 0) {
                    strcpy(perm, "r--");
                } else if (strcmp(perm_value, "5") == 0) {
                    strcpy(perm, "r-x");
                } else if (strcmp(perm_value, "6") == 0) {
                    strcpy(perm, "rw-");
                } else if (strcmp(perm_value, "7") == 0) {
                    strcpy(perm, "rwx");
                }
                fprintf(stderr, "%d %s %d %s %s\n", dir_entry.firstBlock, perm, dir_entry.size, time_str, dir_entry.name);
            }
        }
        fat_value = fat[fat_value];
    }

    return 0;
}

int chmod(const char* mode, const char* fs_name) {
    if (fs_fd == -1) {
        fprintf(stderr, "No filesystem is mounted\n");
        return -1;
    }

    // Find file
    size_t num_entries = block_size / sizeof(directory_entry);
    directory_entry dir_entry;
    int root_fat_value = 1;

    while(root_fat_value != 0xFFFF) {
        off_t root_dir_offset = lseek(fs_fd, fat_size + ((root_fat_value - 1) * block_size), SEEK_SET);
        if (root_dir_offset == (off_t)-1) {
            fprintf(stderr, "Failed to seek to root directory\n");
            return -1;
        }

        for (size_t i = 0; i < num_entries; i++) {
            ssize_t read_bytes = read(fs_fd, &dir_entry, sizeof(directory_entry));
            if (read_bytes == -1) {
                fprintf(stderr, "Error reading directory entry\n");
                return -1;
            }

            // Found file
            if (strncmp(dir_entry.name, fs_name, sizeof(dir_entry.name)) == 0) {
                // Save position of fs_name 
                off_t current_pos = lseek(fs_fd, 0, SEEK_CUR);

                // Start at original permission
                uint8_t new_perm = dir_entry.perm;

                // Parse mode
                for (int i = 1; mode[i] != '\0'; i++) {
                    uint8_t perm_change = 0;

                    // Parse for perm change
                    switch (mode[i]) {
                        case 'r': perm_change = 4; break;
                        case 'w': perm_change = 2; break;
                        case 'x': perm_change = 1; break;
                        default: fprintf(stderr, "Invalid mode\n"); return -1;
                    }

                    // Apply change
                    switch (mode[0]) {
                        case '+': new_perm |= perm_change; break;
                        case '-': new_perm &= ~perm_change; break;
                        case '=': new_perm = perm_change; break;
                        default: fprintf(stderr, "Invalid operator\n"); return -1;
                    }
                }

                // Prevent setting permissions to 1 (execute only) and 3 (write and execute)
                if (new_perm == 1 || new_perm == 3) {
                    fprintf(stderr, "Invalid resulting permission\n");
                    return -1;
                } else {
                    dir_entry.perm = new_perm;
                    // fprintf(stderr, "Value: %u\n", dir_entry.perm);
                }

                // Write out directory entry
                lseek(fs_fd, current_pos - sizeof(directory_entry), SEEK_SET); // Go back to the file entry
                write(fs_fd, &dir_entry, sizeof(directory_entry)); // Write to file to delete the entry
                return 0;
            }
        }
        root_fat_value = fat[root_fat_value];
    }

    fprintf(stderr, "File not found\n");
    return -1;
}
