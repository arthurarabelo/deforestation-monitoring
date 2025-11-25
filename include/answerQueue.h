#ifndef ANSWER_QUEUE_H
#define ANSWER_QUEUE_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "communication.h"

typedef struct answer_node {
    answer_t* data;
    struct answer_node *next;
} answer_node_t;

/* FIFO QUEUE */
typedef struct {
    answer_node_t *front;
    answer_node_t *rear;
} answer_queue_t;

// Function to initialize the queue
void initialize_answer_queue(answer_queue_t* q);

// Function to check if the queue is empty
int is_answer_queue_empty(answer_queue_t* q);

// Function to get the element at the front of the queue (Peek operation)
answer_node_t* peek_answer(answer_queue_t* q);

// Function to add an element to the queue (Enqueue operation)
void enqueue_answer(answer_queue_t* q, answer_t* data);

// Function to remove an element from the queue (Dequeue operation)
void dequeue_answer(answer_queue_t* q);

// Function to free all the nodes of the queue
void free_answer_queue(answer_queue_t* q);

#endif