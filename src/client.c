#include "communication.h"
#include "utils.h"

#define MAX_CITY_NAME 50
#define SERVERPORT "8080"

int main(int argc, char *argv[]){
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
    char *protocol;

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
    } else if(strcmp(protocol, "v6") == 0){
        hints.ai_family = AF_INET6;
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

    Graph* forest_graph = createGraph(N);

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

    rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo);
	if (rv != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
	close(sockfd);

	return 0;

    freeGraph(forest_graph);

    return 0;
}