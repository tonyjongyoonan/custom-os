#include <stdio.h>
#include <string.h>
#include "errors.h"

const char* pennos_errors[] = {
    "File was not found",
    "Permission denied",
    "Command was not found",
    "Process was not found",
    "Process was not spawned",
    "Argument was not found",
    "Signal was not found",
    "Process was already waited on",
    "Status was not found",
    "PCB was not found",
    "Error with Process status"
    "Too many files open", 
    "No space to allocate Deque",
    "Invalid file descriptor error",
    "File write error",
    "Deque space errors",
    "Prompt error",
    "File read error",
    "File is opened in another process",
    "Wrong Process waited on"
    // Add corresponding strings for additional error types
    // ...
};

const char* mapEnumToString(pennos_error error) {
    if (error >= 0 && error < NumberOfErrors) {
        return pennos_errors[error];
    } else {
        return "UnknownError"; 
    }
}