/**
 * @file f_pennos.h
 * @brief Header file for our f functions.
 *
 * This file defines functions and data structures for a simple file system
 * implementation, including file operations like open, close, read, write,
 * and directory-related operations.
 */

#include <signal.h>     // sigaction, sigemptyset, sigfillset, signal
#include <stdio.h>  // dprintf, fputs, perror
#include <stdlib.h>     // malloc, free
#include <sys/time.h>   // setitimer
#include <ucontext.h>   // getcontext, makecontext, setcontext, swapcontext
#include <unistd.h>     // read, usleep, write
#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "pcb.h"
#include "parser.h"
#include "pennfat.h"

/**
 * @def MAX_OPEN_FILES
 * @brief Maximum number of open files allowed in the file system.
 */
#define MAX_OPEN_FILES 128

/**
 * @def STDIN_FD
 * @brief File descriptor value for standard input.
 */
#define STDIN_FD 0

/**
 * @def STDOUT_FD
 * @brief File descriptor value for standard output.
 */
#define STDOUT_FD 1

/**
 * @def F_WRITE
 * @brief File write mode constant.
 */
#define F_WRITE 1

/**
 * @def F_READ
 * @brief File read mode constant.
 */
#define F_READ 2

/**
 * @def F_APPEND
 * @brief File append mode constant.
 */
#define F_APPEND 3

/**
 * @def F_SEEK_SET
 * @brief File seek set mode constant.
 */
#define F_SEEK_SET 0

/**
 * @def F_SEEK_CUR
 * @brief File seek current mode constant.
 */
#define F_SEEK_CUR 1

/**
 * @def F_SEEK_END
 * @brief File seek end mode constant.
 */
#define F_SEEK_END 2

/**
 * @enum fd_type
 * @brief Enumeration representing the type of file descriptor.
 */
typedef enum {
    FD_UNINIT,  /**< Uninitialized file descriptor. */
    FD_STDIN,   /**< Standard input file descriptor. */
    FD_STDOUT,  /**< Standard output file descriptor. */
    FD_FILE     /**< Regular file descriptor. */
} fd_type;

typedef struct {
    directory_entry dir_entry; /**< Directory entry associated with the file. */
    int offset;               /**< Current offset in the file. */
    uint8_t mode;             /**< File access mode (1 for read, 2 for write, 3 for append). */
    fd_type fd_type;          /**< Type of file descriptor. */
    int ref_count;            /**< Reference count for the file descriptor. */
} FileDescriptor;

// Function prototypes

/**
 * @brief Open a file with the specified mode.
 *
 * @param fname The name of the file to be opened.
 * @param mode The mode of opening the file (F_WRITE, F_READ, F_APPEND).
 *
 * @return Returns a file descriptor on success, or a negative value on error.
 *         Error codes:
 *         -1: Failed to open file.
 *
 */
int f_open(const char *fname, int mode);

/**
 * @brief Close the file referenced by the file descriptor.
 *
 * @param fd The file descriptor of the open file.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_close(int fd);

/**
 * @brief Remove the specified file.
 *
 * @param fname The name of the file to be removed.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_unlink(const char* fname);

/**
 * @brief Read bytes from the file referenced by the file descriptor.
 *
 * @param fd The file descriptor of the open file.
 * @param n The number of bytes to read.
 * @param buf The buffer to store the read bytes.
 *
 * @return Returns the number of bytes read on success, 0 if EOF is reached,
 *         or a negative number on error.
 */
int f_read(int fd, int n, char *buf);

/**
 * @brief Write bytes to the file referenced by the file descriptor.
 *
 * @param fd The file descriptor of the open file.
 * @param str The string containing the bytes to write.
 * @param n The number of bytes to write.
 *
 * @return Returns the number of bytes written on success, or a negative value on error.
 */
int f_write(int fd, const char *str, int n);

/**
 * @brief Reposition the file pointer for the specified file descriptor.
 *
 * @param fd The file descriptor of the open file.
 * @param offset The offset relative to the specified whence.
 * @param whence The reference position for repositioning (F_SEEK_SET, F_SEEK_CUR, F_SEEK_END).
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_lseek(int fd, int offset, int whence);

/**
 * @brief Mounts a PennFAT filesystem by loading its FAT into memory.
 *
 * @param fs_name The name of the filesystem to be mounted.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_mount(const char *fs_name);

/**
 * @brief Creates or updates the timestamp of the specified files.
 *
 * @param cmd A parsed command structure containing information about the 'touch' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_touch(struct parsed_command *cmd);

/**
 * @brief Removes the specified file or files from the filesystem.
 *
 * @param fs_name The name of the file or files to be removed.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_rm(const char *fs_name);

/**
 * @brief Renames a source file to a destination file in the filesystem.
 *
 * @param src The source file to be renamed.
 * @param dst The destination file name.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_mv(const char *src, const char *dst);

/**
 * @brief Copies files from the filesystem to a destination in the host OS.
 *
 * @param cmd A parsed command structure containing information about the 'cp' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_cp(struct parsed_command *cmd);

/**
 * @brief Concatenates and prints files to stdout or overwrites/creates an output file.
 *
 * @param cmd A parsed command structure containing information about the 'cat' command.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_cat(struct parsed_command *cmd);

/**
 * @brief List the specified file or all files in the current directory.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_ls();

/**
 * @brief Changes the permissions of the specified filesystem.
 *
 * @param mode The new permissions mode.
 * @param fs_name The name of the filesystem.
 *
 * @return Returns 0 on success, or a negative value on failure.
 */
int f_chmod(const char* mode, const char* fs_name);

/**
 * @brief Move the file position indicator to a specified location in the file.
 *
 * @param stream  A pointer to the FILE structure representing the file.
 * @param offset  The number of bytes to offset from the specified location.
 * @param whence  The reference position for the offset (SEEK_SET, SEEK_CUR,
 *                or SEEK_END).
 * @return        Upon success, it returns 0; otherwise, it returns -1
 *                to indicate an error.
 */
int f_seek(FILE *stream, long int offset, int whence);

/**
 * @brief Reads a line from the specified stream and stores it into
 *        a string.
 *
 * @param str     Pointer to the destination array where the line is stored.
 * @param n       Maximum number of characters to be read (including null
 *                character).
 * @param stream  A pointer to the FILE structure representing the file.
 * @return        On success, `str` is returned; on error, NULL is returned.
 */
char* f_gets(char *str, int n, FILE *stream);

/**
 * @brief Search for a file with the specified name from our file system
 *        and populate the result in the provided directory_entry structure.
 *
 * @param fname   The name of the file to search for.
 * @param result  Pointer to the directory_entry structure to store info.
 * @return        Upon success, it returns location of directory; otherwise, it returns -1
 */
int f_find_file(const char *fname, directory_entry *result);

/**
 * @brief Tokenize a string into substrings based on the specified
 *        delimiter characters (`delim`).
 *
 * @param str    The string to be tokenized.
 * @param delim  The set of delimiter characters.
 * @return       A pointer to the next token in the string; NULL if no more
 *               tokens are found.
 */
char* f_strtok(char *str, const char *delim);

/**
 * @brief A wrapper function for fprintf that allows formatted output to a stream.
 *        It behaves identically to fprintf, taking a file stream, a format string,
 *        and a variable number of additional arguments for formatting.
 *
 * @param stream The file stream where the output is to be written. This can be
 *               any output stream, including standard output (stdout) or standard
 *               error (stderr).
 * @param format A format string that specifies how subsequent arguments are
 *               converted for output. This string can contain text to be printed
 *               and format specifiers, which are replaced by the values of
 *               the additional arguments.
 * @param ...    A variable number of additional arguments. The number and types
 *               of these arguments should correspond to the format specifiers
 *               in the format string.
 *
 * @return The number of characters written (excluding the null byte used to
 *         end output to strings) if successful; a negative number if an error
 *         occurs.
 *
 */

 int f_fprintf(FILE *stream, const char *format, ...);