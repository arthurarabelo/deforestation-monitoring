#include "threadsUtils.h"

// Definitions of global variables
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
uint16_t mission_city_id = 0;
uint16_t mission_crew_id = 0;
uint16_t th4_busy = 0;   // 0 = livre, 1 = ocupada

void* foward_message(void* data){
    args_dispatcher* args = (args_dispatcher *) data;

    struct sockaddr_in their_addr;
    socklen_t addr_len = sizeof(their_addr);

    while (1) {
        answer_t ans;
        receive_message(args->sockfd, &their_addr, &addr_len, &ans, args->graph);

        switch (ans.h.tipo) {
            case MSG_EQUIPE_DRONE: {
                pthread_mutex_lock(&mutex_equipe_drone);
                    
                enqueue_answer(args->answer_queue_t3_drone, &ans);
                pthread_cond_signal(&cond_equipe_drone);
                
                pthread_mutex_unlock(&mutex_equipe_drone);

                break;
            }
            case MSG_ACK: {
                /* multiplex by each kind of ack (0 or 1) */
                if(ans.p_ack.status == 0){
                    pthread_mutex_lock(&mutex_ack_telemetry);
                    
                    enqueue_answer(args->answer_queue_t2, &ans);
                    pthread_cond_signal(&cond_ack_telemetry);
                    
                    pthread_mutex_unlock(&mutex_ack_telemetry);
                } else if (ans.p_ack.status == 2){
                    pthread_mutex_lock(&mutex_ack_conclusao);
                    
                    enqueue_answer(args->answer_queue_t3_ack, &ans);
                    pthread_cond_signal(&cond_ack_conclusao);
                    
                    pthread_mutex_unlock(&mutex_ack_conclusao);
                }

                break;
            }
            default:
                printf("Tipo desconhecido!\n");
                break;
        }
    }

    return NULL;
}

void* modify_telemetry_data(void* data) {
    while (1) {
        pthread_mutex_lock(&mutex_build_telemetry); /* Critical region */ 
        
        args_t1* args = (args_t1 *) data;
        
        // rand() % 100 gives a number from 0 to 99
        // < 3 yields 3% chance
        for(int i = 0; i < args->len; i++){
            args->arr[i] = (rand() % 100) < 3 ? 1 : 0;
        }
        
        pthread_mutex_unlock(&mutex_build_telemetry);

        sleep(1);
    }   
}

void* send_telemetry(void* data){
    args_t2* args = (args_t2 *) data;

    pthread_mutex_lock(&mutex_build_telemetry);

    printf("[ENVIANDO TELEMETRIA]\n");
    printf("Total de cidades: %d\n", args->len);
    
    /* copy the status of the cities to the telemetry payload */
    for (int i = 0; i < args->len; i++) {
        if(args->arr[i] == 1){
            printf("ALERTA: %s (ID=%d)\n", args->graph->vertices[i].name, i);
        }
        args->payload->dados[i].status = args->arr[i];
    }

    pthread_mutex_unlock(&mutex_build_telemetry);

    while (1) {
        int attempts = 0;
        int received = 0;

        /* internal loop to control attemps and 5s timeout */
        while (attempts < 3 && !received) {

            send_message(args->sockfd, args->p, 1, args->payload, sizeof(payload_telemetria_t));
            printf("Telemetria enviada (tentativa %d/3)\n", attempts + 1);

            pthread_mutex_lock(&mutex_ack_telemetry);

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;

            while (is_answer_queue_empty(args->ack_queue)) {

                /* free mutex and put thread to sleep until the signal wakes it up */
                int ret = pthread_cond_timedwait(&cond_ack_telemetry, &mutex_ack_telemetry, &ts);
                if (ret == ETIMEDOUT) {
                    attempts++;
                    break;
                }
            }

            if (!is_answer_queue_empty(args->ack_queue)) {
                free(pop_answer(args->ack_queue));
                received = 1;
                printf("ACK recebido do servidor\n");
            }

            pthread_mutex_unlock(&mutex_ack_telemetry);
        }

        /* wait until next 30s cycle */
        sleep(30);
    }

    return NULL;
}

void* confirm_crew_received(void* data){
    args_t3* args = (args_t3 *) data;
    int id_cidade = -1;
    int id_equipe = -1;

    while (1) {
        pthread_mutex_lock(&mutex_equipe_drone);

        /* waits until there is a answer in the queue of the MSG_EQUIPE_DRONE answers */
        while(is_answer_queue_empty(args->answer_queue_t3_drone)){
            pthread_cond_wait(&cond_equipe_drone, &mutex_equipe_drone);
        }

        answer_node_t* answer = pop_answer(args->answer_queue_t3_drone);
        id_cidade = answer->data->p_drone.id_cidade;
        id_equipe = answer->data->p_drone.id_equipe;

        free(answer);

        pthread_mutex_unlock(&mutex_equipe_drone);

        // check if th4 is busy
        pthread_mutex_lock(&mutex_t4_state);
        int busy = th4_busy;
        pthread_mutex_unlock(&mutex_t4_state);

        if (busy) {
            send_message(args->sockfd, args->p, 2, args->payload, sizeof(payload_ack_t));
            printf("ACK enviado ao servidor\n");
            printf("Já existe missão ativa, ordem ignorada\n");
            continue;
        }
        
        send_message(args->sockfd, args->p, 2, args->payload, sizeof(payload_ack_t));
        printf("ACK enviado ao servidor\n");

        /* th3 and th4 communication - BEGIN */
        pthread_mutex_lock(&mutex_t3_t4);

        mission_available = 1;
        mission_city_id = id_cidade;
        mission_crew_id = id_equipe;
        printf("Missão registrada para execução\n");
        pthread_cond_signal(&cond_t3_t4);

        while(!mission_completed){
            pthread_cond_wait(&cond_t4_t3, &mutex_t3_t4);
        }

        mission_completed = 0;

        pthread_mutex_unlock(&mutex_t3_t4);
        /* th3 and th4 communication - END */

        /* send MSG_CONCLUSAO to server */
        payload_conclusao_t p_conclusao = {id_cidade, id_equipe};
        send_message(args->sockfd, args->p, 4, &p_conclusao, sizeof(payload_conclusao_t));

        /* waits until server acknoledges the conclusion */
        pthread_mutex_lock(&mutex_ack_conclusao);
        
        while(is_answer_queue_empty(args->answer_queue_t3_ack)){
            pthread_cond_wait(&cond_ack_conclusao, &mutex_ack_conclusao);
        }

        pthread_mutex_unlock(&mutex_ack_conclusao);
    }

    return NULL;
}

void* simulate_drones(void* data){

    args_t4* args = (args_t4 *) data;

    while(1){
        /* th4 and th3 communication - BEGIN */
        pthread_mutex_lock(&mutex_t3_t4);
    
        while (!mission_available) {
            pthread_cond_wait(&cond_t3_t4, &mutex_t3_t4);
        }

        mission_available = 0; 

        // now th4 is busy
        pthread_mutex_lock(&mutex_t4_state);
        th4_busy = 1;
        pthread_mutex_unlock(&mutex_t4_state);
        
        pthread_mutex_unlock(&mutex_t3_t4);

        printf("[MISSÃO EM ANDAMENTO]\n");
        printf("Equipe %s atuando em %s\n", args->graph->vertices[mission_crew_id].name, args->graph->vertices[mission_city_id].name);

        int simulation_seconds = rand() % 31;
        printf("Tempo estimado: %d segundos\n", simulation_seconds);

        /* simulate the action of the drone crew */
        sleep(simulation_seconds);

        pthread_mutex_lock(&mutex_t3_t4);
        
        mission_completed = 1;
        printf("Missão concluída");
        pthread_cond_signal(&cond_t4_t3);
    
        pthread_mutex_unlock(&mutex_t3_t4);
        /* th4 and th3 communication - END */

        // now th4 is free
        pthread_mutex_lock(&mutex_t4_state);
        th4_busy = 0;
        pthread_mutex_unlock(&mutex_t4_state);
    }

    return NULL;
}
