#include "answerQueue.h"

void initialize_answer_queue(answer_queue_t* q) {
    q->front = NULL;
    q->rear = NULL;
}

int is_answer_queue_empty(answer_queue_t* q) { 
    return q->front == NULL; 
}

answer_node_t* peek_answer(answer_queue_t* q) {
    if (is_answer_queue_empty(q)) {
        printf("Queue is empty\n");
        return NULL; // return some default value or handle
                   // error differently
    }
    return q->front;
}

void enqueue_answer(answer_queue_t* q, answer_t* data) {
    answer_node_t* new_element = (answer_node_t *) calloc(1, sizeof(answer_node_t));
    *(new_element->data) = *data;
    
    if (is_answer_queue_empty(q)) {
        q->front = q->rear = new_element;
        return;
    }

    q->rear->next = new_element;
    q->rear = new_element;
}

void dequeue_answer(answer_queue_t* q) {
    if (is_answer_queue_empty(q)) {
        printf("Queue is empty\n");
        return;
    }

    answer_node_t* tmp = q->front;
    q->front = tmp->next;

    if (q->front == NULL){
        q->rear = NULL;
    }

    free(tmp);
}

void free_answer_queue(answer_queue_t* q){
    answer_node_t* current = q->front;

    while (current) {
        answer_node_t* tmp = current;
        current = current->next;
        free(tmp);
    }

    q->front = NULL;
    q->rear = NULL;
}
