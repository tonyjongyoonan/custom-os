/**
 * @file stress.h
 * @brief Header file for stress testing.
 *
 * This file contains declarations and definitions related to stress testing in the system.
 */

#ifndef STRESS_H
#define STRESS_H

/**
 * @brief Causes the calling thread to enter an infinite loop, effectively hanging.
 *
 * This function is designed to make the calling thread enter an infinite loop,
 * causing it to hang indefinitely. It serves as a simple way to stress test a thread
 * that does not exit on its own and needs to be terminated externally.
 */
void hang(void);

/**
 * @brief Runs a non-blocking operation, allowing the calling thread to proceed without hanging.
 *
 * This function is designed to perform a non-blocking operation, allowing the calling
 * thread to proceed without entering a hanging state. It is useful when you want to
 * stress test an operation without blocking the execution flow of the calling thread.
 */
void nohang(void);

/**
 * @brief Recursively spawns itself 26 times, naming the spawned processes Gen_A through Gen_Z.
 *
 * The `recur` function recursively spawns itself 26 times, generating processes
 * named Gen_A through Gen_Z. Each spawned process is block-waited by its parent.
 */
void recur(void);

#endif /* STRESS_H */
