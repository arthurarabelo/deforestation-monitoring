#include "utils.h"
#include "threadsUtils.h"
#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>

#define MAX_CITY_NAME 50
#define SERVERPORT "8080"

void handle_sigint_threads(int sig) {
    (void)sig;
    running = 0;

    pthread_cond_broadcast(&cond_ack_telemetry);
    pthread_cond_broadcast(&cond_ack_conclusao);
    pthread_cond_broadcast(&cond_equipe_drone);
    pthread_cond_broadcast(&cond_t3_t4);
    pthread_cond_broadcast(&cond_t4_t3);
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
    char *protocol, *server_ip;
    char s[INET6_ADDRSTRLEN];

    signal(SIGINT, handle_sigint_threads);

    pthread_t thread1, thread2, thread3, thread4, th_dispatcher; /* thread variables */
    answer_queue_t answer_queue_t2;
    answer_queue_t answer_queue_t3_ack;
    answer_queue_t answer_queue_t3_drone;

    /* initialize mutexes, cond variables and answer queues */
    pthread_mutex_init(&mutex_build_telemetry, NULL);
    pthread_mutex_init(&mutex_ack_telemetry, NULL);
    pthread_mutex_init(&mutex_ack_conclusao, NULL);
    pthread_mutex_init(&mutex_equipe_drone, NULL);
    pthread_mutex_init(&mutex_t3_t4, NULL);
    pthread_mutex_init(&mutex_t4_state, NULL);

    pthread_cond_init(&cond_ack_telemetry, NULL);
    pthread_cond_init(&cond_ack_conclusao, NULL);
    pthread_cond_init(&cond_equipe_drone, NULL);
    pthread_cond_init(&cond_t3_t4, NULL);
    
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

        // Make socket non-blocking to allow checking 'running' flag
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("Cliente conectado ao servidor %s:%s\n", s, SERVERPORT);

    /* lê o arquivo do grafo */
    FILE *file = fopen("grafoamazonialegal.txt", "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    int N, M;
    fscanf(file, "%d %d", &N, &M);

    Graph* forest_graph = createGraph(N);
    
    payload_ack_t p_ack_drone = {1};
    payload_telemetria_t p_telemetry = {0};
    p_telemetry.total = N;
    
    // Initialize all telemetry data entries to zero first
    for (int i = 0; i < MAX_SIZE_DADOS_TELEMETRIA; i++) {
        p_telemetry.dados[i].id_cidade = 0;
        p_telemetry.dados[i].status = 0;
    }
    
    // read vertices and initialize telemetry
    for (int i = 0; i < N; i++) {
        int city_id, city_type;
        char city_name[MAX_CITY_NAME];
        fscanf(file, "%d %99[^0-9] %d", &city_id, city_name, &city_type);
        setVertexInfo(forest_graph, i, city_name, city_type, city_id);
        
        telemetria_t telemetry = {city_id, 0};
        p_telemetry.dados[city_id] = telemetry;
    }
    
    fclose(file);
    
    /* global array shared by threads */
    int telemetry_data[N];
    memset(telemetry_data, 0, sizeof(telemetry_data)); // Initialize to zero

    pthread_barrier_init(&barrier, NULL, 5);
    
	printf("Iniciando threads...\n");
    args_dispatcher data_dispatcher = {sockfd, &answer_queue_t2, &answer_queue_t3_ack, &answer_queue_t3_drone, forest_graph};
    pthread_create(&th_dispatcher, NULL, &foward_message, &data_dispatcher);
    
	printf("[Thread Monitoramento] Iniciada\n");
    args_t1 data_t1 = {telemetry_data, N};
    pthread_create(&thread1, NULL, &modify_telemetry_data, &data_t1);
    
	printf("[Thread Telemetria] Iniciada\n");
    args_t2 data_t2 = {telemetry_data, N, sockfd, p, &p_telemetry, &answer_queue_t2, forest_graph};
    pthread_create(&thread2, NULL, &send_telemetry, &data_t2);

	printf("[Thread Recepção Drones] Iniciada\n");
    args_t3 data_t3 = {sockfd, p, &p_ack_drone, &answer_queue_t3_ack, &answer_queue_t3_drone, forest_graph};
    pthread_create(&thread3, NULL, &confirm_crew_received, &data_t3);
    
	printf("[Thread Simulação Drones] Iniciada\n");
    args_t4 data_t4 = {forest_graph};
    pthread_create(&thread4, NULL, &simulate_drones, &data_t4);
    
	printf("Todas as threads iniciadas com sucesso\n");
	printf("Pressione Ctrl+C para encerrar\n\n");

    while (running){
        sleep(1);
    }
    
    /* Waits for threads to finish */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(th_dispatcher, NULL);
    
    printf("Liberando recursos...\n");

    pthread_barrier_destroy(&barrier);
    
    /* Destroys mutexes and condition variables */
    pthread_mutex_destroy(&mutex_build_telemetry);
    pthread_mutex_destroy(&mutex_ack_telemetry);
    pthread_mutex_destroy(&mutex_ack_conclusao);
    pthread_mutex_destroy(&mutex_equipe_drone);
    pthread_mutex_destroy(&mutex_t3_t4);
    pthread_mutex_destroy(&mutex_t4_state);

    pthread_cond_destroy(&cond_ack_telemetry);
    pthread_cond_destroy(&cond_ack_conclusao);
    pthread_cond_destroy(&cond_equipe_drone);
    pthread_cond_destroy(&cond_t3_t4);
    pthread_cond_destroy(&cond_t4_t3);

    /* frees answer queues */
    free_answer_queue(&answer_queue_t2);
    free_answer_queue(&answer_queue_t3_ack);
    free_answer_queue(&answer_queue_t3_drone);

    freeGraph(forest_graph);
    freeaddrinfo(servinfo);
    close(sockfd);
    
    printf("Cliente encerrado com sucesso.\n");
    return 0;
}