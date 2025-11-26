#ifndef ALERT_QUEUE_H
#define ALERT_QUEUE_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/* FIFO QUEUE */

typedef struct alert_event {
    int id_cidade;
    int equipe_atuando; // 0 = no crew in action, 1 = there is a crew in action
    time_t timestamp;
    struct alert_event *next;
} alert_event_t;

typedef struct {
    alert_event_t *front;
    alert_event_t *rear;
} alert_queue_t;

// Function to initialize the queue
void initialize_alert_queue(alert_queue_t* q);

// Function to check if the queue is empty
int is_alert_queue_empty(alert_queue_t* q);

// Function to get the element at the front of the queue (pop operation)
alert_event_t* pop_alert(alert_queue_t* q);

// Function to get the element at the front of the queue (peek operation)
alert_event_t* peek_alert(alert_queue_t* q);

// Function to check if element is already on the queue
int has_alert_element(alert_queue_t* q, int id_cidade);

// Function to add an element to the queue (Enqueue operation)
void enqueue_alert(alert_queue_t* q, int id_cidade, int equipe_atuando, time_t timestamp);

// Function to remove an element from the queue (Dequeue operation)
void dequeue_alert(alert_queue_t* q);

// Function to free all the nodes of the queue
void free_alert_queue(alert_queue_t* q);

#endif