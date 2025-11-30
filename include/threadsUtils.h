#ifndef THREADS_UTILS_H
#define THREADS_UTILS_H

#include "communication.h"
#include "answerQueue.h"
#include "graph.h"
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <stdatomic.h>

// External declarations (definitions are in threadsUtils.c)
extern pthread_mutex_t mutex_build_telemetry;
extern pthread_mutex_t mutex_ack_telemetry;
extern pthread_mutex_t mutex_ack_conclusao;
extern pthread_mutex_t mutex_equipe_drone;
extern pthread_mutex_t mutex_t4_state;
extern pthread_mutex_t mutex_t3_t4;

extern pthread_cond_t cond_ack_telemetry;
extern pthread_cond_t cond_ack_conclusao;
extern pthread_cond_t cond_equipe_drone;
extern pthread_cond_t cond_t3_t4;
extern pthread_cond_t cond_t4_t3;

extern uint16_t mission_available;
extern uint16_t mission_completed;
extern uint16_t th4_busy;   // 0 = livre, 1 = ocupada

extern pthread_barrier_t barrier;

extern volatile sig_atomic_t running;

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
    Graph* graph;
} args_t2;

typedef struct {
    int sockfd;
    struct addrinfo *p;
    payload_ack_t *payload;
    answer_queue_t* answer_queue_t3_ack;
    answer_queue_t* answer_queue_t3_drone;
    Graph* graph;
} args_t3;

typedef struct {
    Graph* graph;
} args_t4;

typedef struct {
    int sockfd;
    answer_queue_t* answer_queue_t2;
    answer_queue_t* answer_queue_t3_ack;
    answer_queue_t* answer_queue_t3_drone;
    Graph* graph;
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