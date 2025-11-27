#include "communication.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void host_to_network_short_header(header_t *header){
    header->tamanho = htons(header->tamanho);
    header->tipo = htons(header->tipo);
}

void network_to_host_short_header(header_t *header){
    header->tamanho = ntohs(header->tamanho);
    header->tipo = ntohs(header->tipo);
}

void host_to_network_long_payload(void *payload, MessageType type){
    switch (type) {
        case MSG_TELEMETRIA: {
            payload_telemetria_t *p = (payload_telemetria_t*) payload;
            for(int i = 0; i < MAX_SIZE_DADOS_TELEMETRIA; i++){
                p->dados[i].id_cidade = htonl(p->dados[i].id_cidade);
                p->dados[i].status = htonl(p->dados[i].status);
            }
            p->total = htonl(p->total);
            break;
        }
        case MSG_ACK: {
            payload_ack_t *p = (payload_ack_t*) payload;
            p->status = htonl(p->status);
            break;
        }
        case MSG_EQUIPE_DRONE: {
            payload_equipe_drone_t *p = (payload_equipe_drone_t*) payload;
            p->id_cidade = htonl(p->id_cidade);
            p->id_equipe = htonl(p->id_equipe);
            break;
        }
        case MSG_CONCLUSAO: {
            payload_conclusao_t *p = (payload_conclusao_t*) payload;
            p->id_cidade = htonl(p->id_cidade);
            p->id_equipe = htonl(p->id_equipe);
            break;
        }
    }
}

void network_to_host_long_payload(void *payload, MessageType type){
    switch (type) {
        case MSG_TELEMETRIA: {
            payload_telemetria_t *p = (payload_telemetria_t*) payload;
            for(int i = 0; i < MAX_SIZE_DADOS_TELEMETRIA; i++){
                p->dados[i].id_cidade = ntohl(p->dados[i].id_cidade);
                p->dados[i].status = ntohl(p->dados[i].status);
            }
            p->total = ntohl(p->total);
            break;
        }
        case MSG_ACK: {
            payload_ack_t *p = (payload_ack_t*) payload;
            p->status = ntohl(p->status);
            break;
        }
        case MSG_EQUIPE_DRONE: {
            payload_equipe_drone_t *p = (payload_equipe_drone_t*) payload;
            p->id_cidade = ntohl(p->id_cidade);
            p->id_equipe = ntohl(p->id_equipe);
            break;
        }
        case MSG_CONCLUSAO: {
            payload_conclusao_t *p = (payload_conclusao_t*) payload;
            p->id_cidade = ntohl(p->id_cidade);
            p->id_equipe = ntohl(p->id_equipe);
            break;
        }
    }
}

void send_message(int sockfd, struct addrinfo *p, MessageType type, void *payload, size_t payload_size){
    char buffer[MAX_BUFFER_SIZE];
	unsigned long int numbytes;
    header_t header;
    header.tipo = type;
    header.tamanho = payload_size;
    size_t total_size = sizeof(header) + payload_size;
    
    // prevent endianess problemss
    host_to_network_short_header(&header);
    host_to_network_long_payload(payload, type);

    // Copia o header e o payload pro buffer
    memcpy(buffer, &header, sizeof(header_t));
    memcpy(buffer + sizeof(header_t), payload, payload_size);

    // Envia tudo num único datagrama
    if((numbytes = sendto(sockfd, buffer, total_size, 0, (struct sockaddr*) p->ai_addr, p->ai_addrlen)) == (long unsigned int) -1){
        perror("sendto");
		exit(1);
    }

	printf("talker: sent %ld bytes\n", numbytes);

    network_to_host_long_payload(payload, type);
}

void receive_message(int sockfd, struct sockaddr_in *their_addr, socklen_t *addrlen, answer_t *answer, Graph* graph) {
    char buffer[MAX_BUFFER_SIZE];
	unsigned long int numbytes;
    char s[INET6_ADDRSTRLEN];

    if((numbytes = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *) their_addr, addrlen)) == (long unsigned int) -1){
        perror("recvfrom");
		exit(1);
    }

    if (numbytes < sizeof(header_t)){
        /* message incomplete for sure */
        answer->ack = 0;
        return;
    }

    header_t header;
    memcpy(&header, buffer, sizeof(header_t));
    network_to_host_short_header(&header);
    
    answer->h = header;

    /* check if the message or some part of it was not lost using the header->tamanho */
    if(header.tamanho > numbytes - sizeof(header_t)){
        fprintf(stderr, "Pacotes foram perdidos na mensagem: esperado %d, recebido %zd\n", header.tamanho, numbytes - sizeof(header_t));
        answer->ack = 0;
        return;
    }
    
    switch (header.tipo) {
        case MSG_TELEMETRIA: {
            payload_telemetria_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            answer->p_telemetry = p;
            answer->ack = 1;

            printf("[TELEMETRIA RECEBIDA]");
            printf("Total de cidades monitoradas: %d\n", answer->p_telemetry.total);
            for (int i = 0; i < answer->p_telemetry.total; i++) {
                if(answer->p_telemetry.dados[i].status == 1){
                    printf("ALERTA: %s (ID=%d)\n", graph->vertices[i].name, i);
                }
            }
            break;
        }
        case MSG_ACK: {
            payload_ack_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            answer->p_ack = p;
            answer->ack = 1;

            printf("ACK: %d\n", p.status);
            break;
        }
        case MSG_EQUIPE_DRONE: {
            payload_equipe_drone_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            answer->p_drone = p;
            answer->ack = 1;

            printf("[ORDEM DE DRONE RECEBIDA]\n");
            printf("Cidade: %s (ID=%d)\n", graph->vertices[answer->p_drone.id_cidade].name, answer->p_drone.id_cidade);
            printf("Equipe: %s (ID=%d)\n", graph->vertices[answer->p_drone.id_equipe].name, answer->p_drone.id_equipe);
            break;
        }
        case MSG_CONCLUSAO: {
            payload_conclusao_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);
            
            answer->p_conclusion = p;
            answer->ack = 1;

            printf("Equipe %d concluiu ajuda em cidade %d\n", p.id_equipe, p.id_cidade);
            break;
        }
        default:
            printf("Tipo desconhecido!\n");
            break;
    }    
}
