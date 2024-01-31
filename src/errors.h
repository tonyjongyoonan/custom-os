/**
 * @file errors.h
 * @brief Header file for handling errors in PennOS.
 *
 * This file provides declarations and definitions related to error handling
 * in PennOS, including the enumeration of error types and necessary dependencies.
 */

#ifndef ERRORS_H
#define ERRORS_H



#include <stdio.h>
#include <string.h>

/**
 * @brief Enumeration of error types in PennOS.
 *
 * This enumeration lists various error types that can occur in PennOS,
 * providing a convenient way to handle different error scenarios.
 *
 * - `FileNotFoundError`: File not found error.
 * - `PermissionError`: Permission error.
 * - `CommandNotFoundError`: Command not found error.
 * - `ProcessNotFoundError`: Process not found error.
 * - `ProcessSpawnError`: Process spawn error.
 * - `ArgumentNotFoundError`: Argument not found error.
 * - `SignalNotFoundError`: Signal not found error.
 * - `ProcessWaitError`: Process wait error.
 * - `StatusNotFoundError`: Status not found error.
 * - `PCBNotFoundError`: Process Control Block (PCB) not found error.
 * - `ProcessStatusError`: Process status error.
 * - `TooManyFilesOpenError`: Too many files open error.
 * - `NoMoreSpaceError`: No more space error.
 * - `InvalidFileDescriptorError`: Invalid file descriptor error.
 * - `FileWriteError`: File write error.
 * - `DequeSpaceError`: Deque space error.
 * - `PromptError`: Prompt error.
 * - `FileReadError`: File read error.
 * - `FileIsOpenError`: File is open error.
 * - `NumberOfErrors`: Keep track of errors (do not add errors below this point).
 */
typedef enum {
    FileNotFoundError,
    PermissionError,
    CommandNotFoundError,
    ProcessNotFoundError,
    ProcessSpawnError,
    ArgumentNotFoundError,
    SignalNotFoundError,
    ProcessWaitError,
    StatusNotFoundError,
    PCBNotFoundError,
    ProcessStatusError,
    TooManyFilesOpenError,
    NoMoreSpaceError,
    InvalidFileDescriptorError,
    FileWriteError,
    DequeSpaceError,
    PromptError,
    FileReadError,
    FileIsOpenError,
    WrongProcessWaitedError,
    //add errors here
    NumberOfErrors //keep track of errors
} pennos_error;


/**
 * @brief External declaration of the array of PennOS error messages.
 *
 * This external constant array, `pennos_errors`, provides human-readable
 * error messages corresponding to each error type in the PennOS system.
 * It is expected to be defined elsewhere in the codebase.
 */
extern const char* pennos_errors[];

/**
 * @brief Maps a PennOS error enumeration to its corresponding string representation.
 *
 * This function takes a PennOS error enumeration and returns a pointer to the
 * corresponding human-readable string representation of the error.
 *
 * @param error The PennOS error enumeration to be mapped.
 * @return const char* A pointer to the string representation of the specified error.
 */
const char* mapEnumToString(pennos_error error);

#endif /* ERRORS_H */