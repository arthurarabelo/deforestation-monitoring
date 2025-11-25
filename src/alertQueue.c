#include "alertQueue.h"

void initialize_alert_queue(alert_queue_t* q) {
    q->front = NULL;
    q->rear = NULL;
}

int is_alert_queue_empty(alert_queue_t* q) { 
    return q->front == NULL; 
}

alert_event_t* peek_alert(alert_queue_t* q) {
    if (is_alert_queue_empty(q)) {
        printf("Queue is empty\n");
        return NULL; // return some default value or handle
                   // error differently
    }
    return q->front;
}

int has_alert_element(alert_queue_t* q, int id_cidade){
    alert_event_t* current = q->front;

    while (current) {
        if(current->id_cidade == id_cidade) return 1;
        current = current->next;
    }

    return 0;
}


void enqueue_alert(alert_queue_t* q, int id_cidade, int equipe_atuando, time_t timestamp) {
    alert_event_t* new_element = (alert_event_t *) calloc(1, sizeof(alert_event_t));
    new_element->id_cidade = id_cidade;
    new_element->equipe_atuando = equipe_atuando;
    new_element->timestamp = timestamp;
    new_element->next = NULL;
    
    if (is_alert_queue_empty(q)) {
        q->front = q->rear = new_element;
        return;
    }

    q->rear->next = new_element;
    q->rear = new_element;
}

void dequeue_alert(alert_queue_t* q) {
    if (is_alert_queue_empty(q)) {
        printf("Queue is empty\n");
        return;
    }

    alert_event_t* tmp = q->front;
    q->front = tmp->next;

    if (q->front == NULL){
        q->rear = NULL;
    }

    free(tmp);
}

void free_alert_queue(alert_queue_t* q){
    alert_event_t* current = q->front;

    while (current) {
        alert_event_t* tmp = current;
        current = current->next;
        free(tmp);
    }

    q->front = NULL;
    q->rear = NULL;
}
