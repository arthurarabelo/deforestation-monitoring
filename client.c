#include "communication.h"
#include "utils.h"
#include "answerQueue.h"
#include <pthread.h>

#define MAX_CITY_NAME 50
#define SERVERPORT "8080"

pthread_mutex_t mutex_build_telemetry;
pthread_mutex_t mutex_ack_telemetry;
pthread_mutex_t mutex_ack_drone;
pthread_mutex_t mutex_equipe_drone;

pthread_cond_t cond_ack_telemetry;

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
    answer_queue_t* answer_queue_t2;
    answer_queue_t* answer_queue_t3_ack;
    answer_queue_t* answer_queue_t3_drone;
} args_dispatcher;

/* dispatcher thread 

th1: -
th2: receive MSG_ACK (0)
th3: receive MSG_EQUIPE_DRONE and MSG_ACK (1)
th4: receive missions from th3 (by shared queue --> use mutex)

*/

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
                } else if (ans.p_ack.status == 1){
                    pthread_mutex_lock(&mutex_ack_drone);
                    
                    enqueue_answer(args->answer_queue_t3_ack, &ans);
                    
                    pthread_mutex_unlock(&mutex_ack_drone);
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
                peek_answer(args->ack_queue);
                received = 1;
            }

            pthread_mutex_unlock(&mutex_ack_telemetry);
        }

        /* wait till next 30s cycle */
        sleep(30);
    }

    return NULL;
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
    char *protocol, *server_ip;

    pthread_t thread1, thread2, thread3, thread4, th_dispatcher; /* thread variables */
    answer_queue_t answer_queue_t2;
    answer_queue_t answer_queue_t3_ack;
    answer_queue_t answer_queue_t3_drone;

    /* initialize mutexes, cond variables and answer queues */
    pthread_mutex_init(&mutex_build_telemetry, NULL);
    pthread_mutex_init(&mutex_ack_telemetry, NULL);
    pthread_mutex_init(&mutex_ack_drone, NULL);
    pthread_mutex_init(&mutex_equipe_drone, NULL);
    pthread_cond_init(&cond_ack_telemetry, NULL);
    initialize_answer_queue(&answer_queue_t2);
    initialize_answer_queue(&answer_queue_t3_ack);
    initialize_answer_queue(&answer_queue_t3_drone);
    
    if (argc != 2) {
        fprintf(stderr, "how to use it: %s <v4|v6>\n", argv[0]);
        exit(1);
    }
    
    protocol = argv[1];
    
    memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
    if(strcmp(protocol, "v4") == 0){
        hints.ai_family = AF_INET;
        server_ip = "127.0.0.1";
    } else if(strcmp(protocol, "v6") == 0){
        hints.ai_family = AF_INET6;
        server_ip = "::1";
    } else {
        hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
    }
    
    FILE *file = fopen("grafo_random.txt", "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }
    
    int N, M;
    fscanf(file, "%d %d", &N, &M);
    
    payload_telemetria_t p_telemetry = {0};
    p_telemetry.total = N;
    
    // read vertices and initialize telemetry
    for (int i = 0; i < N; i++) {
        int city_id, city_type;
        char city_name[MAX_CITY_NAME];
        fscanf(file, "%d %s %d", &city_id, city_name, &city_type);
        
        telemetria_t telemetry = {city_id, 0};
        p_telemetry.dados[city_id] = telemetry;
    }
    
    fclose(file);
    
    rv = getaddrinfo(server_ip, SERVERPORT, &hints, &servinfo);
	if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    
    /* global array shared by threads */
    int telemetry_data[N];

    args_t1 data_t1 = {telemetry_data, N};
    pthread_create(&thread1, NULL, &modify_telemetry_data, &data_t1);

    args_t2 data_t2 = {telemetry_data, N, sockfd, p, &p_telemetry, &answer_queue_t2};
    pthread_create(&thread2, NULL, &send_telemetry, &data_t2);

    args_dispatcher data_dispatcher = {sockfd, &answer_queue_t2, &answer_queue_t3_ack, &answer_queue_t3_drone};
    pthread_create(&th_dispatcher, NULL, &foward_message, &data_dispatcher);

    /* TODO: implement thread3 and th4 */
    
    /* Waits for threads to finish */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(th_dispatcher, NULL);
    
    /* Destroys mutexes and condition variables */
    pthread_mutex_destroy(&mutex_build_telemetry);
    pthread_mutex_destroy(&mutex_ack_telemetry);
    pthread_mutex_destroy(&mutex_ack_drone);
    pthread_mutex_destroy(&mutex_equipe_drone);
    pthread_cond_destroy(&cond_ack_telemetry);

    freeaddrinfo(servinfo);

    /* frees answer queues */
    free_answer_queue(&answer_queue_t2);
    free_answer_queue(&answer_queue_t3_ack);
    free_answer_queue(&answer_queue_t3_drone);

    close(sockfd);
    return 0;
}