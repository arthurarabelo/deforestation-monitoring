#ifndef THREADS_UTILS_H
#define THREADS_UTILS_H

#include "communication.h"
#include "answerQueue.h"
#include <pthread.h>

pthread_mutex_t mutex_build_telemetry;
pthread_mutex_t mutex_ack_telemetry;
pthread_mutex_t mutex_ack_conclusao;
pthread_mutex_t mutex_equipe_drone;
pthread_mutex_t mutex_t4_state;
pthread_mutex_t mutex_t3_t4;

pthread_cond_t cond_ack_telemetry;
pthread_cond_t cond_ack_conclusao;
pthread_cond_t cond_equipe_drone;
pthread_cond_t cond_t3_t4;
pthread_cond_t cond_t4_t3;

uint16_t mission_available = 0;
uint16_t mission_completed = 0;
uint16_t th4_busy = 0;   // 0 = livre, 1 = ocupada

typedef struct {
    int *arr;
    int len;
} args_t1;

typedef struct {
    int *arr;
    int len;
    int sockfd;
    struct addrinfo *p;
    payload_telemetria_t *payload;
    answer_queue_t* ack_queue;
} args_t2;

typedef struct {
    int sockfd;
    struct addrinfo *p;
    payload_ack_t *payload;
    answer_queue_t* answer_queue_t3_ack;
    answer_queue_t* answer_queue_t3_drone;
} args_t3;

typedef struct {
    int sockfd;
    answer_queue_t* answer_queue_t2;
    answer_queue_t* answer_queue_t3_ack;
    answer_queue_t* answer_queue_t3_drone;
} args_dispatcher;

/* dispatcher thread 
th1: -
th2: receive MSG_ACK (0)
th3: receive MSG_EQUIPE_DRONE and MSG_ACK (2)
th4: receive missions from th3 (by shared queue --> use mutex)
*/
void* foward_message(void* data);

void* modify_telemetry_data(void* data);

void* send_telemetry(void* data);

void* confirm_crew_received(void* data);

void* simulate_drones(void* data);

#endif