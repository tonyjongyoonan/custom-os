#include <stdio.h>
#include <stdlib.h> 
#include "Deque.h"

Deque* Deque_Allocate() {
  Deque* deq = malloc(sizeof(Deque));
  deq->front = NULL;
  deq->back = NULL;
  deq->num_elements = 0;
  return deq;
}

void Deque_Free(Deque *dq) {
  if(dq) {
    PCB* ptr = dq->front->pcb;
  while(dq->num_elements > 0) {
    Deque_Pop_Front(dq, &ptr);
  }
  free(dq);
  }
}

int Deque_Size(Deque *dq) {
  return dq->num_elements;
}

void Deque_Push_Front(Deque *dq, PCB* payload) {
  DequeNode *node = malloc(sizeof(DequeNode));
  if (node == NULL) { //there's no space for a new DequeNode
    exit(EXIT_FAILURE);
  }
  node->prev = NULL;
  node->pcb = payload;
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

bool Deque_Pop_Front(Deque *dq, PCB **payload_ptr) {
  if (!dq->front) { //equivalent to if (dq->front == NULL)
    return false;
  }
  DequeNode* frontNode = dq->front;
  if (dq->num_elements == 1) {
    dq->front = NULL;
    dq->back = NULL;
  } else {
    dq->front = frontNode->next;
    dq->front->prev = NULL;
  }
  // *payload_ptr = *(frontNode->pcb);
  *payload_ptr = frontNode->pcb;
  dq->num_elements--;
  free(frontNode);
  return true;
}

bool Deque_Pop_PID(Deque *dq, pid_t pid_to_pop) {
  if (dq->front == NULL) {
    return false;
  }
  DequeNode* current = dq->front;
  while (current != NULL) {
      if (current->pcb->pid == pid_to_pop) {
          bool res = Deque_Pop_Pcb(dq, current);
          // printf("kms\n");
          return res;
      }
      current = current->next;
  }
    return false;
}

bool Deque_Pop_Pcb(Deque *dq, DequeNode* pcb) {
  if (dq->front == NULL) {
    return false;
  }
  PCB* pcb_temp = NULL;
  if (!pcb->prev) {
    Deque_Pop_Front(dq, &pcb_temp);
  } else if (!pcb->next) {
    Deque_Pop_Back(dq, &pcb_temp);
  } else {
    // not head or tail
    pcb->prev->next = pcb->next;
    pcb->next->prev = pcb->prev;
    dq->num_elements--;
    free(pcb);
  }
  return true;
}

void Deque_Push_Back(Deque *dq, PCB* payload) {
  DequeNode *node = malloc(sizeof(DequeNode));
  if (node == NULL) {
    //there's no space for a new DequeNode
    exit(EXIT_FAILURE);
  }
  node->pcb = payload;
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

bool Deque_Pop_Back(Deque *dq, PCB** payload_ptr) {
  if (dq->num_elements == 0) return false;
  DequeNode *lastNode = dq->back;
  if (dq->num_elements == 1) {
    dq->front = NULL;
    dq->back = NULL;
  } else {
    dq->back = lastNode->prev;
    dq->back->next = NULL;
  }
  *payload_ptr = lastNode->pcb;

  dq->num_elements--;
  free(lastNode);
  return true;
}




