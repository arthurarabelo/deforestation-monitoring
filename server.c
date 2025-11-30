#include "utils.h"
#include <signal.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <errno.h>

#define MYPORT "8080"	// the port users will be connecting to
#define MAX_CITY_NAME 50

static volatile sig_atomic_t running = 1;

void handle_sigint_server(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
	char *protocol, *server_ip;
    
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
    
	if ((rv = getaddrinfo(server_ip, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
    
	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        /* makes socket nonblocking */
        fcntl(sockfd, F_SETFL, O_NONBLOCK);

        // lose the pesky "Address already in use" error message
        int yes = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    
    printf("Servidor escutando na porta 8080...\n");
    printf("Pressione Ctrl+C para encerrar\n\n");
    
    // Register signal handler for graceful shutdown
    signal(SIGINT, handle_sigint_server);
    
	freeaddrinfo(servinfo);
    
    FILE *file = fopen("grafoamazonialegal.txt", "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    int N, M;
    fscanf(file, "%d %d", &N, &M);

    Graph* forest_graph = createGraph(N);
    int dist[N];

    alert_queue_t alerts;
    initialize_alert_queue(&alerts);

    // read vertices
    for (int i = 0; i < N; i++) {
        int city_id, city_type;
        char city_name[MAX_CITY_NAME];
        fscanf(file, "%d %99[^0-9] %d", &city_id, city_name, &city_type);
        setVertexInfo(forest_graph, i, city_name, city_type, city_id);
    }

    // read edges (graph is undirected, so add edges in both directions)
    for (int i = 0; i < M; i++) {
        int u, v, weight;
        fscanf(file, "%d %d %d", &u, &v, &weight);
        addEdge(forest_graph, u, v, weight);
        addEdge(forest_graph, v, u, weight); // Add reverse edge for undirected graph
    }

    fclose(file);

    while (running) {
        answer_t msg_recvd;
        
        // Try to receive message (non-blocking)
        receive_message(sockfd, &their_addr, &addr_len, &msg_recvd, forest_graph);
        
        if(msg_recvd.ack == 0){
            /* keeps listening until client transmit again */
            continue;
        } else {
            switch (msg_recvd.h.tipo) {
                case MSG_TELEMETRIA: {
                    handle_telemetry(&msg_recvd.p_telemetry, &alerts);

                    /* SEND ACK */
                    payload_ack_t ack = {0};
                    send_message(sockfd, (struct sockaddr *)&their_addr, addr_len, MSG_ACK, &ack, sizeof(payload_ack_t));
                    printf("ACK enviado (tipo=0)\n\n");
    
                    alert_event_t* current_alert = peek_alert(&alerts);
                    
                    while (current_alert) {
                        if(current_alert->equipe_atuando == 0){
                            int nearestAvailableDroneCrew = findNearestAvailableDroneCrew(forest_graph, current_alert->id_cidade, dist);
                            
                            if(nearestAvailableDroneCrew != -1){
                                printf("[DESPACHANDO DRONES]\n");
                                printf("Cidade em alerta: %s (ID=%d)\n", forest_graph->vertices[current_alert->id_cidade].name, current_alert->id_cidade);
                                printf("Dijkstra: capital %s (ID=%d) selecionada, distância = %d km\n", forest_graph->vertices[nearestAvailableDroneCrew].name, nearestAvailableDroneCrew, dist[nearestAvailableDroneCrew]);

                                forest_graph->vertices[nearestAvailableDroneCrew].available = 0;
                                current_alert->equipe_atuando = 1;
        
                                payload_equipe_drone_t equipe = {current_alert->id_cidade, nearestAvailableDroneCrew};
                                send_message(sockfd, (struct sockaddr *)&their_addr, addr_len, MSG_EQUIPE_DRONE, &equipe, sizeof(payload_equipe_drone_t));
    
                                printf("Ordem enviada: Equipe %s (ID=%d) -> Cidade %s (ID=%d)\n\n", forest_graph->vertices[nearestAvailableDroneCrew].name, nearestAvailableDroneCrew,
                                forest_graph->vertices[current_alert->id_cidade].name,current_alert->id_cidade );
                            }
                        }
                        current_alert = current_alert->next;
                    }
                    break;
                }
                case MSG_ACK: {
                    /* if the ACK is ACK_EQUIPE_DRONE, unqueue the alerts first element (drone was already received) */
                    if(msg_recvd.p_ack.status == 1){
                        printf("[ACK RECEBIDO]\n");
                        alert_event_t* current_alert = peek_alert(&alerts);
                        if(current_alert){
                            printf("Cliente confirmou recebimento de ordem de drone para %s\n\n", forest_graph->vertices[current_alert->id_cidade].name);
                        }
                    }
                    break;
                }
                case MSG_CONCLUSAO: {
                    /* mark the crew as available on its corresponding vertex */
                    int crew = msg_recvd.p_conclusion.id_equipe;
                    forest_graph->vertices[crew].available = 1;
                    printf("Equipe %s liberada para novas missões\n\n", forest_graph->vertices[crew].name);

                    // remove o alerta da fila
                    free(pop_alert(&alerts));

                    payload_ack_t ack = {2};
                    send_message(sockfd, (struct sockaddr *)&their_addr, addr_len, MSG_ACK, &ack, sizeof(payload_ack_t));
                    printf("ACK enviado (tipo=2)\n\n");
                    break;
                }
            }
        }
    }
    
    printf("\n[Encerrando servidor...]\n");
    printf("Liberando recursos...\n");
    
	close(sockfd);
    free_alert_queue(&alerts);
    freeGraph(forest_graph);
    
    printf("Servidor encerrado com sucesso.\n");
	return 0;
}
