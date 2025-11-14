#include "communication.h"
#include "utils.h"

/* TODO: move this file to root */

#define MYPORT "8080"	// the port users will be connecting to

#define MAXBUFLEN 100

int main(int argc, char *argv[]) {
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	// int numbytes;
	// struct sockaddr_storage their_addr;
	// char buf[MAXBUFLEN];
	// socklen_t addr_len;
	// char s[INET6_ADDRSTRLEN];
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

	// addr_len = sizeof their_addr;
	// if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
	// 	(struct sockaddr *)&their_addr, &addr_len)) == -1) {
	// 	perror("recvfrom");
	// 	exit(1);
	// }

	receive_message(sockfd);

	close(sockfd);

	return 0;
}
