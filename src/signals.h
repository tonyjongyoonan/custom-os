/**
 * @file signals.h
 * @brief Header file for PennOS signals.
 *
 * This file defines the PennOS signals enumeration, which includes signal types
 * such as S_SIGSTOP, S_SIGCONT, and S_SIGTERM.
 */

#ifndef SIGNALS_H
#define SIGNALS_H
/**
 * @brief Enumeration of PennOS signals.
 *
 * This enumeration lists various signal types in PennOS, including:
 * - S_SIGSTOP: Signal to stop a process.
 * - S_SIGCONT: Signal to continue a stopped process.
 * - S_SIGTERM: Signal to terminate a process.
 */
typedef enum {
    S_SIGSTOP, /**< Signal to stop a process. */
    S_SIGCONT, /**< Signal to continue a stopped process. */
    S_SIGTERM  /**< Signal to terminate a process. */
} pennos_signal;

#endif /* SIGNALS_H */