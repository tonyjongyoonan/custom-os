/**
 * @file Deque.h
 * @brief Header file for a deque.
 *
 * This file defines structures and functions for a generic deque.
 */

#ifndef DEQUE
#define DEQUE


#include <stdbool.h>    // for bool type (true, false)
#include "pcb.h"

/**
 * @brief Structure representing a node in a doubly-linked deque.
 *
 * This structure defines a node in a doubly-linked deque, containing a pointer to a Process Control Block (PCB)
 * representing the current job, and pointers to the next and previous nodes in the deque.
 *
 * @param pcb   Pointer to the Process Control Block (PCB) representing the current job.
 * @param next  Pointer to the next node in the deque, or NULL if this is the last node.
 * @param prev  Pointer to the previous node in the deque, or NULL if this is the first node.
 */
typedef struct dq_node_st {
  PCB* pcb;                    ///< Pointer to the Process Control Block (PCB) representing the current job.
  struct dq_node_st* next;     ///< Pointer to the next node in the deque, or NULL if this is the last node.
  struct dq_node_st* prev;     ///< Pointer to the previous node in the deque, or NULL if this is the first node.
} DequeNode;

/**
 * @brief Structure representing a deque.
 *
 * This structure contains metadata about the deque, including the number of elements,
 * a pointer to the beginning of the deque (`front`), and a pointer to the end of the deque (`back`).
 *
 * @param num_elements   The number of elements in the deque.
 * @param front          Pointer to the beginning of the deque, or NULL if the deque is empty.
 * @param back           Pointer to the end of the deque, or NULL if the deque is empty.
 */
typedef struct dq_st {
  int num_elements;          ///< The number of elements in the deque.
  DequeNode* front;          ///< Pointer to the beginning of the deque, or NULL if the deque is empty.
  DequeNode* back;           ///< Pointer to the end of the deque, or NULL if the deque is empty.
} Deque;

/**
 * @brief Allocates space for a Deque and its variables.
 *
 * This function dynamically allocates memory for a Deque structure and its associated variables,
 * including `num_elements`, `front`, and `back`. The allocated Deque is initialized with default values.
 *
 * @return Deque* Pointer to the allocated Deque structure.
 */
Deque* Deque_Allocate(void);

/**
 * @brief Frees the memory allocated for a Deque.
 *
 * This function frees the memory previously allocated for a Deque structure by `Deque_Allocate`.
 * After calling this function, it is unsafe to use the deque referenced by the `dq` parameter.
 *
 * @param dq Pointer to the Deque structure to be freed.
 */
void Deque_Free(Deque *dq);

/**
 * @brief Returns the number of elements in the deque.
 *
 * This function returns the number of elements in the deque referenced by the `dq` parameter.
 *
 * @param dq Pointer to the Deque structure to query.
 * @return int The size of the deque.
 */
int Deque_Size(Deque *dq);

/**
 * @brief Adds a new element to the front of the Deque.
 *
 * This function pushes a new element with the specified payload to the front of the deque
 * referenced by the `dq` parameter.
 *
 * @param dq      Pointer to the Deque structure to push onto.
 * @param payload Pointer to the payload to push to the front.
 * @return void
 */
void Deque_Push_Front(Deque *dq, PCB* payload);

/**
 * @brief Pops an element from the front of the deque.
 *
 * This function pops an element from the front of the deque referenced by the `dq` parameter.
 * On success, the popped node's payload is returned through the `payload_ptr` parameter.
 *
 * @param dq          Pointer to the Deque structure to pop from.
 * @param payload_ptr A return parameter; on success, the popped node's payload is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty) and true on success.
 */
bool Deque_Pop_Front(Deque *dq, PCB** payload_ptr);

/**
 * @brief Pops an element with a specific PID from the deque.
 *
 * This function pops an element with the specified PID from the deque referenced by the `dq` parameter.
 *
 * @param dq          Pointer to the Deque structure to pop from.
 * @param pid_to_pop  The PID of the element to pop.
 * @return bool       Returns false on failure (e.g., the deque is empty or the specified PID is not found),
 *                    and true on success.
 */
bool Deque_Pop_PID(Deque *dq, pid_t pid_to_pop);

/**
 * @brief Peeks at the element at the front of the deque.
 *
 * This function retrieves the element at the front of the deque referenced by the `dq` parameter.
 * On success, the peeked node's payload is returned through the `payload_ptr` parameter.
 *
 * @param dq          Pointer to the Deque structure to peek.
 * @param payload_ptr A return parameter; on success, the peeked node's payload is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool Deque_Peek_Front(Deque *dq, PCB** payload_ptr);

/**
 * @brief Pushes a new element to the end of the deque.
 *
 * This function pushes a new element with the specified payload to the end of the deque
 * referenced by the `dq` parameter. This is the "end" version of `Deque_Push_Front`.
 *
 * @param dq      Pointer to the Deque structure to push onto.
 * @param payload Pointer to the payload to push to the end.
 */
void Deque_Push_Back(Deque *dq, PCB* payload);

/**
 * @brief Pops an element from the end of the deque.
 *
 * This function pops an element from the end of the deque referenced by the `dq` parameter.
 * On success, the popped node's payload is returned through the `payload_ptr` parameter.
 * This is the "end" version of `Deque_Pop_Front`.
 *
 * @param dq          Pointer to the Deque structure to remove from.
 * @param payload_ptr A return parameter; on success, the popped node's payload is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool Deque_Pop_Back(Deque *dq, PCB** payload_ptr);

/**
 * @brief Peeks at the element at the back of the deque.
 *
 * This function retrieves the element at the back of the deque referenced by the `dq` parameter.
 * On success, the peeked node's payload is returned through the `payload_ptr` parameter.
 *
 * @param dq          Pointer to the Deque structure to peek.
 * @param payload_ptr A return parameter; on success, the peeked node's payload is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool Deque_Peek_Back(Deque *dq, PCB** payload_ptr);

/**
 * @brief Pops a specified PCB node from the deque.
 *
 * This function pops the specified PCB node from the deque referenced by the `dq` parameter.
 *
 * @param dq   Pointer to the Deque structure to pop from.
 * @param pcb  Pointer to the PCB node to pop.
 * @return bool Returns false on failure (e.g., the specified PCB node is not found), and true on success.
 */
bool Deque_Pop_Pcb(Deque *dq, DequeNode* pcb);

#endif /* DEQUE */
