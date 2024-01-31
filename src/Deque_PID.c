#include <stdio.h>
#include <stdlib.h> 
#include "Deque_PID.h"

pid_Deque* pid_Deque_Allocate() {
  pid_Deque* curr_shell_pids = malloc(sizeof(pid_Deque));
  curr_shell_pids->front = NULL;
  curr_shell_pids->back = NULL;
  curr_shell_pids->num_elements = 0;
  return curr_shell_pids;
}

void pid_Deque_Free(pid_Deque *dq) {
  if(dq) {
    pid_t ptr = dq->front->pid;
    while(dq->num_elements > 0) {
      pid_Deque_Pop_Front(dq, &ptr);
    }
    free(dq);
  }
}

int pid_Deque_Size(pid_Deque *dq) {
  return dq->num_elements;
}

void pid_Deque_Push_Front(pid_Deque *dq, pid_t payload) {
  pid_DequeNode *node = malloc(sizeof(pid_DequeNode));
  if (node == NULL) { //there's no space for a new DequeNode
    exit(EXIT_FAILURE);
  }
  node->prev = NULL;
  node->pid = payload;
  if (dq->num_elements != 0) {
    dq->front->prev = node;
    node->next = dq->front;
    dq->front = node;

  } else {
    dq->front = node;
    dq->back = node;
    node->next = NULL;
  }
  dq->num_elements++;

}

bool pid_Deque_Pop_Front(pid_Deque *dq, pid_t *payload_ptr) {
  if (!dq->front) { //equivalent to if (dq->front == NULL)
    return false;
  }
  pid_DequeNode* frontNode = dq->front;
  if (dq->num_elements == 1) {
    dq->front = NULL;
    dq->back = NULL;
  } else {
    dq->front = frontNode->next;
    dq->front->prev = NULL;
  }
  // *payload_ptr = *(frontNode->pcb);
  *payload_ptr = frontNode->pid;

  dq->num_elements--;
  free(frontNode);
  return true;
}

bool pid_Deque_Pop_PID(pid_Deque *dq, pid_t pid_to_pop) {
  if (dq->front == NULL) {
    return false;
  }
  pid_DequeNode* current = dq->front;
  while (current != NULL) {
      if (current->pid == pid_to_pop) {
        return pid_Deque_Pop_Node(dq, current);
      }
      current = current->next;
  }
  return false;
}

bool pid_Deque_Pop_Node(pid_Deque *dq, pid_DequeNode* node) {
  if (dq->front == NULL) {
    return false;
  }
  pid_t* pid_temp = malloc(sizeof(pid_t));
  
  if (!node->prev) {
    // printf("a\n");
    pid_Deque_Pop_Front(dq, pid_temp);
  } else if (!node->next) {
    // printf("b\n");
    pid_Deque_Pop_Back(dq, pid_temp);
  } else {
    // not head or tail
    // printf("c\n");
    node->prev->next = node->next;
    node->next->prev = node->prev;
    dq->num_elements--;
    // free(node);
  }
  free(pid_temp);
  return true;
}

void pid_Deque_Push_Back(pid_Deque *dq, pid_t payload) {
  pid_DequeNode *node = malloc(sizeof(pid_DequeNode));
  if (node == NULL) {
    //there's no space for a new DequeNode
    exit(EXIT_FAILURE);
  }
  node->pid = payload;
  node->next = NULL;
  if (dq->num_elements == 0) {
    dq->front = node;
    dq->back = node;
    node->prev = NULL;
  } else {
    dq->back->next = node;
    node->prev = dq->back;
    dq->back = node;
  }
  dq->num_elements++;
 }

bool pid_Deque_Pop_Back(pid_Deque *dq, pid_t* payload_ptr) {
  if (dq->num_elements == 0) return false;
  pid_DequeNode *lastNode = dq->back;
  if (dq->num_elements == 1) {
    dq->front = NULL;
    dq->back = NULL;
  } else {
    dq->back = lastNode->prev;
    dq->back->next = NULL;
  }
  *payload_ptr = lastNode->pid;

  dq->num_elements--;
  free(lastNode);
  return true;
}




