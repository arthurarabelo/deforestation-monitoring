#include "communication.h"

void host_to_network_long_header(header_t *header){
    header->tamanho = htonl(header->tamanho);
    header->tipo = htonl(header->tipo);
}

void network_to_host_long_header(header_t *header){
    header->tamanho = ntohl(header->tamanho);
    header->tipo = ntohl(header->tipo);
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

void send_message(int sockfd, struct sockaddr_in *dest, MessageType type, void *payload, size_t payload_size){
    header_t header;
    header.tipo = type;
    header.tamanho = payload_size;
    
    size_t total_size = sizeof(header_t) + payload_size;
    uint8_t buffer[total_size];
    
    host_to_network_long_header(&header);
    host_to_network_long_payload(payload, type);

    // Copia o header e o payload pro buffer
    memcpy(buffer, &header, sizeof(header_t));
    memcpy(buffer + sizeof(header_t), payload, payload_size);

    // Envia tudo num único datagrama
    sendto(sockfd, buffer, total_size, 0, (struct sockaddr*) dest, sizeof(*dest));

    network_to_host_long_payload(payload, type);
}

void receive_message(int sockfd) {
    uint8_t buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in src;
    socklen_t addrlen = sizeof(src);

    unsigned long int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&src, &addrlen);
    if (n < sizeof(header_t)){
        /* TODO: message incomplete */
    }

    header_t *header = (header_t*) buffer;
    void *payload = buffer + sizeof(header_t);

    /* TODO: check if the message or some part of it was not lost using the header->tamanho */

    printf("Recebido tipo=%d, tamanho=%d bytes\n", header->tipo, header->tamanho);

    switch (header->tipo) {
        case MSG_TELEMETRIA: {
            payload_telemetria_t *p = (payload_telemetria_t*) payload;
            printf("Total: %d\n", p->total);
            for (int i = 0; i < p->total; i++)
                printf("Cidade %d, status %d\n", p->dados[i].id_cidade, p->dados[i].status);
            break;
        }
        case MSG_ACK: {
            payload_ack_t *p = (payload_ack_t*) payload;
            printf("ACK: %d\n", p->status);
            break;
        }
        case MSG_EQUIPE_DRONE: {
            payload_equipe_drone_t *p = (payload_equipe_drone_t*) payload;
            printf("Equipe %d enviada à cidade %d\n", p->id_equipe, p->id_cidade);
            break;
        }
        case MSG_CONCLUSAO: {
            payload_conclusao_t *p = (payload_conclusao_t*) payload;
            printf("Equipe %d concluiu ajuda em cidade %d\n", p->id_equipe, p->id_cidade);
            break;
        }
        default:
            printf("Tipo desconhecido!\n");
            break;
    }
}
