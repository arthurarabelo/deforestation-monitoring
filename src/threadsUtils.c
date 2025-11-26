#include "threadsUtils.h"

void* foward_message(void* data){
    args_dispatcher* args = (args_dispatcher *) data;

    struct sockaddr_in their_addr;
    socklen_t addr_len = sizeof(their_addr);

    while (1) {
        answer_t ans;
        receive_message(args->sockfd, &their_addr, &addr_len, &ans);

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

    /* copy the status of the cities to the telemetry payload */
    for (int i = 0; i < args->len; i++) {
        args->payload->dados[i].status = args->arr[i];
    }

    pthread_mutex_unlock(&mutex_build_telemetry);

    while (1) {
        int attempts = 0;
        int received = 0;

        /* internal loop to control attemps and 5s timeout */
        while (attempts < 3 && !received) {
            send_message(args->sockfd, args->p, 1, args->payload, sizeof(payload_telemetria_t));

            pthread_mutex_lock(&mutex_ack_telemetry);

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;

            while (is_answer_queue_empty(args->ack_queue)) {

                /* free mutex and put thread to sleep 'till the signal wakes it up */
                int ret = pthread_cond_timedwait(&cond_ack_telemetry, &mutex_ack_telemetry, &ts);
                if (ret == ETIMEDOUT) {
                    attempts++;
                    break;
                }
            }

            if (!is_answer_queue_empty(args->ack_queue)) {
                free(pop_answer(args->ack_queue));
                received = 1;
            }

            pthread_mutex_unlock(&mutex_ack_telemetry);
        }

        /* wait till next 30s cycle */
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

        /* waits 'till there is a answer in the queue of the MSG_EQUIPE_DRONE answers */
        while(is_answer_queue_empty(args->answer_queue_t3_drone)){
            pthread_cond_wait(&cond_equipe_drone, &mutex_equipe_drone);
        }

        answer_t* answer = pop_answer(args->answer_queue_t3_drone);
        id_cidade = answer->p_drone.id_cidade;
        id_equipe = answer->p_drone.id_equipe;
        free(answer);

        pthread_mutex_unlock(&mutex_equipe_drone);

        // check if th4 is busy
        pthread_mutex_lock(&mutex_t4_state);
        int busy = th4_busy;
        pthread_mutex_unlock(&mutex_t4_state);

        if (busy) {
            payload_conclusao_t p = {id_cidade, id_equipe};
            send_message(args->sockfd, args->p, 4, &p, sizeof(p));
            continue;
        }
        
        send_message(args->sockfd, args->p, 2, args->payload, sizeof(payload_ack_t));

        /* th3 and th4 communication - BEGIN */
        pthread_mutex_lock(&mutex_t3_t4);

        mission_available = 1;
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

        /* waits 'till server acknoledges the conclusion */
        pthread_mutex_lock(&mutex_ack_conclusao);
        
        while(is_answer_queue_empty(args->answer_queue_t3_ack)){
            pthread_cond_wait(&cond_ack_conclusao, &mutex_ack_conclusao);
        }

        pthread_mutex_unlock(&mutex_ack_conclusao);
    }

    return NULL;
}

void* simulate_drones(void* data){

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

        /* simulate the action of the drone crew */
        sleep(30);

        pthread_mutex_lock(&mutex_t3_t4);
        
        mission_completed = 1;
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
