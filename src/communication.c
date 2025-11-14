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

void receive_message(int sockfd) {
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in their_addr;
    socklen_t addrlen = sizeof(their_addr);
	unsigned long int numbytes;
    char s[INET6_ADDRSTRLEN];

    if((numbytes = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&their_addr, &addrlen)) == (long unsigned int) -1){
        perror("recvfrom");
		exit(1);
    }

	printf("listener: got packet from %s\n", inet_ntop(their_addr.sin_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
	printf("listener: packet is %ld bytes long\n", numbytes);

    if (numbytes < sizeof(header_t)){
        /* TODO: message incomplete for sure */
    }

    header_t header;
    memcpy(&header, buffer, sizeof(header_t));
    network_to_host_short_header(&header);

    printf("Recebido tipo=%d, tamanho=%d bytes\n", header.tipo, header.tamanho);

    /* TODO: check if the message or some part of it was not lost using the header->tamanho */
    
    switch (header.tipo) {
        case MSG_TELEMETRIA: {
            payload_telemetria_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            printf("Total: %d\n", p.total);
            for (int i = 0; i < p.total; i++)
                printf("Cidade %d, status %d\n", p.dados[i].id_cidade, p.dados[i].status);
            break;
        }
        case MSG_ACK: {
            payload_ack_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            printf("ACK: %d\n", p.status);
            break;
        }
        case MSG_EQUIPE_DRONE: {
            payload_equipe_drone_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            printf("Equipe %d enviada à cidade %d\n", p.id_equipe, p.id_cidade);
            break;
        }
        case MSG_CONCLUSAO: {
            payload_conclusao_t p;
            network_to_host_long_payload(&p, header.tipo);
            memcpy(&p, buffer + sizeof(header_t), header.tamanho);

            printf("Equipe %d concluiu ajuda em cidade %d\n", p.id_equipe, p.id_cidade);
            break;
        }
        default:
            printf("Tipo desconhecido!\n");
            break;
    }    
}
