/**
 * @file Deque_PID.h
 * @brief Header file for a deque with PID.
 *
 * This file defines structures and functions for a deque with Process ID (PID).
 */

#ifndef DEQUE_PID
#define DEQUE_PID


#include <stdbool.h> 
#include <sys/types.h>

/**
 * @brief Structure representing a node in a deque with PID.
 *
 * This structure defines a node in a deque with Process ID (PID), containing a PID,
 * and pointers to the next and previous nodes in the deque.
 *
 * @param pid   The Process ID (PID) of the current job.
 * @param next  Pointer to the next node in the deque, or NULL if this is the last node.
 * @param prev  Pointer to the previous node in the deque, or NULL if this is the first node.
 */
typedef struct pid_dq_node_st {
  pid_t pid;                           ///< The Process ID (PID) of the current job.
  struct pid_dq_node_st* next;         ///< Pointer to the next node in the deque, or NULL if this is the last node.
  struct pid_dq_node_st* prev;         ///< Pointer to the previous node in the deque, or NULL if this is the first node.
} pid_DequeNode;

/**
 * @brief Structure representing a deque with PID.
 *
 * This structure contains metadata about the deque with Process ID (PID),
 * including the number of elements, a pointer to the beginning of the deque (`front`),
 * and a pointer to the end of the deque (`back`).
 *
 * @param num_elements   The number of elements in the deque.
 * @param front          Pointer to the beginning of the deque, or NULL if the deque is empty.
 * @param back           Pointer to the end of the deque, or NULL if the deque is empty.
 */
typedef struct pid_dq_st {
  int num_elements;                    ///< The number of elements in the deque.
  pid_DequeNode* front;                ///< Pointer to the beginning of the deque, or NULL if the deque is empty.
  pid_DequeNode* back;                 ///< Pointer to the end of the deque, or NULL if the deque is empty.
} pid_Deque;


/**
 * @brief Allocates space for a deque with PID and initializes its variables.
 *
 * This function dynamically allocates memory for a deque with Process ID (PID) structure,
 * including variables like `num_elements`, `front`, and `back`. The allocated deque is initialized with default values.
 *
 * @return pid_Deque* Pointer to the allocated deque with PID structure.
 */
pid_Deque* pid_Deque_Allocate(void);

/**
 * @brief Frees the memory allocated for a deque with PID.
 *
 * This function frees the memory previously allocated for a deque with Process ID (PID) structure
 * by `pid_Deque_Allocate`. After calling this function, it is unsafe to use the deque referenced by the `dq` parameter.
 *
 * @param dq Pointer to the deque with PID structure to be freed.
 */
void pid_Deque_Free(pid_Deque *dq);

/**
 * @brief Returns the number of elements in the deque with PID.
 *
 * This function returns the number of elements in the deque with Process ID (PID) referenced by the `dq` parameter.
 *
 * @param dq Pointer to the deque with PID structure to query.
 * @return int The size of the deque.
 */
int pid_Deque_Size(pid_Deque *dq);

/**
 * @brief Adds a new element to the front of the deque with PID.
 *
 * This function pushes a new element with the specified PID payload to the front of the deque
 * referenced by the `dq` parameter.
 *
 * @param dq      Pointer to the deque with PID structure to push onto.
 * @param payload The PID payload to push to the front.
 */
void pid_Deque_Push_Front(pid_Deque *dq, pid_t payload);

/**
 * @brief Pops an element from the front of the deque with PID.
 *
 * This function pops an element from the front of the deque with Process ID (PID)
 * referenced by the `dq` parameter. On success, the popped node's PID is returned
 * through the `payload_ptr` parameter.
 *
 * @param dq          Pointer to the deque with PID structure to pop from.
 * @param payload_ptr A return parameter; on success, the popped node's PID is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool pid_Deque_Pop_Front(pid_Deque *dq, pid_t* payload_ptr);

/**
 * @brief Pops an element with a specific PID from the front of the deque with PID.
 *
 * This function pops an element with the specified PID from the front of the deque
 * with Process ID (PID) referenced by the `dq` parameter.
 *
 * @param dq          Pointer to the deque with PID structure to pop from.
 * @param pid_to_pop  The PID of the element to pop.
 * @return bool       Returns false on failure (e.g., the deque is empty or the specified PID is not found),
 *                    and true on success.
 */
bool pid_Deque_Pop_PID(pid_Deque *dq, pid_t pid_to_pop);

/**
 * @brief Peeks at the element at the front of the deque with PID.
 *
 * This function retrieves the element at the front of the deque with Process ID (PID)
 * referenced by the `dq` parameter. On success, the peeked node's PID is returned
 * through the `payload_ptr` parameter.
 *
 * @param dq          Pointer to the deque with PID structure to peek.
 * @param payload_ptr A return parameter; on success, the peeked node's PID is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool pid_Deque_Peek_Front(pid_Deque *dq, pid_t* payload_ptr);

/**
 * @brief Pushes a new element to the end of the deque with PID.
 *
 * This function pushes a new element with the specified PID payload to the end of the deque
 * referenced by the `dq` parameter. This is the "end" version of `pid_Deque_Push_Front`.
 *
 * @param dq      Pointer to the deque with PID structure to push onto.
 * @param payload The PID payload to push to the end.
 */
void pid_Deque_Push_Back(pid_Deque *dq, pid_t payload);

/**
 * @brief Pops an element from the end of the deque with PID.
 *
 * This function pops an element from the end of the deque with Process ID (PID)
 * referenced by the `dq` parameter. On success, the popped node's PID is returned
 * through the `payload_ptr` parameter. This is the "end" version of `pid_Deque_Pop_Front`.
 *
 * @param dq          Pointer to the deque with PID structure to remove from.
 * @param payload_ptr A return parameter; on success, the popped node's PID is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool pid_Deque_Pop_Back(pid_Deque *dq, pid_t* payload_ptr);

/**
 * @brief Peeks at the element at the back of the deque with PID.
 *
 * This function retrieves the element at the back of the deque with Process ID (PID)
 * referenced by the `dq` parameter. On success, the peeked node's PID is returned
 * through the `payload_ptr` parameter.
 *
 * @param dq          Pointer to the deque with PID structure to peek.
 * @param payload_ptr A return parameter; on success, the peeked node's PID is returned through this parameter.
 * @return bool       Returns false on failure (e.g., the deque is empty), and true on success.
 */
bool pid_Deque_Peek_Back(pid_Deque *dq, pid_t* payload_ptr);


/**
 * @brief Pops a specified PID node from the deque with PID.
 *
 * This function pops the specified PID node from the deque with Process ID (PID)
 * referenced by the `dq` parameter.
 *
 * @param dq   Pointer to the deque with PID structure to pop from.
 * @param node Pointer to the PID node to pop.
 * @return bool Returns false on failure (e.g., the specified PID node is not found), and true on success.
 */
bool pid_Deque_Pop_Node(pid_Deque *dq, pid_DequeNode* node);

#endif /* DEQUE_PID */
