#include "communication.h"
#include "utils.h"

#define MYPORT "8080"	// the port users will be connecting to
#define MAX_CITY_NAME 50

int main(int argc, char *argv[]) {
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	struct sockaddr_in their_addr;
    socklen_t addr_len = sizeof(their_addr);
	char *protocol, *server_ip;
    
    if (argc != 2) {
        fprintf(stderr, "how to use it: %s <v4|v6>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen("grafo_random.txt", "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    int N, M;
    fscanf(file, "%d %d", &N, &M);

    Graph* forest_graph = createGraph(N);

    alert_queue_t alerts;
    initialize_alert_queue(&alerts);

    // read vertices
    for (int i = 0; i < N; i++) {
        int city_id, city_type;
        char city_name[MAX_CITY_NAME];
        fscanf(file, "%d %s %d", &city_id, city_name, &city_type);

        setVertexInfo(forest_graph, i, city_name, city_type, city_id);
    }

    // read edges
    for (int i = 0; i < M; i++) {
        int u, v, weight;
        fscanf(file, "%d %d %d", &u, &v, &weight);

        addEdge(forest_graph, u, v, weight);
    }

    fclose(file);

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
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

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

	freeaddrinfo(servinfo);

	printf("listener: waiting to recvfrom...\n");

    while (1) {
        answer_t msg_recvd;
        receive_message(sockfd, &their_addr, &addr_len, &msg_recvd);
    
        if(msg_recvd.ack == 0){
            /* keeps listening untill client transmit again */
            continue;
        } else {
            switch (msg_recvd.h.tipo) {
                case MSG_TELEMETRIA: {
                    /* SEND ACK */
                    payload_ack_t ack = {0};
                    send_message(sockfd, p, MSG_ACK, &ack, sizeof(payload_ack_t));
    
                    handle_telemetry(&msg_recvd.p_telemetry, &alerts);
    
                    alert_event_t* current_alert = peek_alert(&alerts);
                    if(current_alert){
                        int nearestAvailableDroneCrew = findNearestAvailableDroneCrew(forest_graph, current_alert->id_cidade);
                        
                        if(nearestAvailableDroneCrew != -1){
                            forest_graph->vertices[nearestAvailableDroneCrew].available = 0;
    
                            payload_equipe_drone_t equipe = {current_alert->id_cidade, nearestAvailableDroneCrew};
                            send_message(sockfd, p, MSG_EQUIPE_DRONE, &equipe, sizeof(payload_equipe_drone_t));
                        }
                    }
                    
                    break;
                }
                case MSG_ACK: {
                    /* if the ACK is ACK_EQUIPE_DRONE, unqueue the alerts first element (drone was already received) */
                    if(msg_recvd.p_ack.status == 1){
                        alert_event_t* current_alert = pop_alert(&alerts);
                        if(current_alert){
                            free(current_alert);
                        }
                    }
                    break;
                }
                case MSG_CONCLUSAO: {
                    /* mark the crew as available on its corresponding vertex */
                    int crew = msg_recvd.p_conclusion.id_equipe;
                    forest_graph->vertices[crew].available = 1;
                    break;
                }
                default:
                    printf("Tipo desconhecido!\n");
                    break;
            }
        }
    }
    
	close(sockfd);
    free_alert_queue(&alerts);
    freeGraph(forest_graph);
	return 0;
}
