/**
 * @file pennfat.h
 * @brief Header file for PennFAT filesystem and related commands.
 *
 * This file defines structures, constants, and function prototypes for
 * interacting with the PennFAT filesystem and implementing various filesystem
 * commands such as mkfs, mount, umount, touch, rm, mv, cp, cat, ls, and chmod.
 */

#ifndef PENN_FAT_H
#define PENN_FAT_H

#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <time.h>
#include "parser.h"

/**
 * @struct directory_entry
 * @brief Structure representing a directory entry in the PennFAT filesystem.
 */
typedef struct {
    char name[32];        /**< Null-terminated file name. */
    uint32_t size;        /**< Number of bytes in the file. */
    uint16_t firstBlock;  /**< The first block number of the file. */
    uint8_t type;         /**< The type of the file. */
    uint8_t perm;         /**< File permissions. */
    time_t mtime;         /**< Creation/modification time. */
    char reserved[16];    /**< Reserved for future use or extra credits. */
} directory_entry;

// Helper functions

/**
 * @brief Find a file in the PennFAT filesystem by name.
 *
 * This function searches for a file with the specified name in the PennFAT filesystem
 * and retrieves its directory entry information.
 *
 * @param fname The name of the file to search for.
 * @param result A pointer to a directory_entry structure to store the result.
 *
 * @return Returns the position of the file on directory on success, 
 * or a negative value on failure.
 */
int find_file(const char* fname, directory_entry *result);

/**
 * @brief Creates a single file in the PennFAT filesystem.
 *
 * @param fs_name The name of the file to create.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int touch_single(const char *fs_name);


/**
 * @brief Concatenates files and writes to a specified output file, overwriting it.
 *
 * This function concatenates the specified files and writes the result to the
 * output file. If the output file does not exist, it will be created.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cat_f_w(struct parsed_command *cmd);

/**
 * @brief Concatenates files and appends to a specified output file.
 *
 * This function concatenates the specified files and appends the result to the
 * output file. If the output file does not exist, it will be created.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cat_f_a(struct parsed_command *cmd);

/**
 * @brief Reads from the terminal and appends to a specified output file.
 *
 * This function reads from the terminal and appends the input to the
 * specified output file. If the output file does not exist, it will be created.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cat_a_f(struct parsed_command *cmd);


/**
 * @brief Reads from the terminal and overwrites a specified output file.
 *
 * This function reads from the terminal and overwrites the specified output file.
 * If the output file does not exist, it will be created.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cat_w_f(struct parsed_command *cmd);

/**
 * @brief Concatenates files and prints the result to stdout.
 *
 * This function concatenates the specified files and prints the result to stdout.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cat_f(struct parsed_command *cmd);


// Function Prototypes for all the penn fat commands

/**
 * @brief Creates a PennFAT filesystem in the file named FS_NAME. 
 * The number of blocks in the FAT region is BLOCKS_IN_FAT 
 * and the block size is 256, 512, 1024, 2048, or 4096 bytes 
 * corresponding to the value (0 through 4) of BLOCK_SIZE_CONFIG.
 *
 * @param fs_name The name of the filesystem to be created.
 * @param blocks_in_fat The number of blocks in the FAT region (1-32)
 * @param block_size_config The block size configuration.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int mkfs(const char *fs_name, int blocks_in_fat, int block_size_config);

/**
 * @brief Mounts a PennFAT filesystem by loading its FAT into memory.
 *
 * @param fs_name The name of the filesystem to be mounted.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int mount(const char *fs_name);

/**
 * @brief Unmounts the currently mounted filesystem.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int umount();

/**
 * @brief Creates or updates the timestamp of the specified files.
 *
 * @param cmd A parsed command structure containing information about the 'touch' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int touch(struct parsed_command *cmd);

/**
 * @brief Removes the specified file or files from the filesystem.
 *
 * @param fs_name The name of the file or files to be removed.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int rm(const char *fs_name);

/**
 * @brief Renames a source file to a destination file in the filesystem.
 *
 * @param src The source file to be renamed.
 * @param dst The destination file name.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int mv(const char *src, const char *dst);

/**
 * @brief Copies files from the filesystem to a destination in the host OS.
 *
 * @param cmd A parsed command structure containing information about the 'cp' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cp(struct parsed_command *cmd);

/**
 * @brief Concatenates and prints files to stdout or overwrites/creates an output file.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int cat_all(struct parsed_command *cmd);

/**
 * @brief List the specified file or all files in the current directory.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int ls();

/**
 * @brief Changes the permissions of the specified filesystem.
 *
 * @param mode The new permissions mode.
 * @param fs_name The name of the filesystem.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int chmod(const char* mode, const char* fs_name);

#endif