#include "communication.h"

void send_telemetria(int sockfd, const struct addrinfo *ai, header_t *header, payload_telemetria_t* payload){
    payload->total = htons(payload->total);
    for(int i = 0; i < MAX_SIZE_DADOS_TELEMETRIA; i++){
        payload->dados[i].id_cidade = htons(payload->dados[i].id_cidade);
        payload->dados[i].status = htons(payload->dados[i].status);
    }

    char buf[sizeof(header_t) + sizeof(payload_telemetria_t)];
    memcpy(buf, header, sizeof(header_t));
    memcpy(buf + sizeof(header), payload, sizeof(payload_telemetria_t));

    sendto(sockfd, buf, strlen(buf), 0, ai->ai_addr, ai->ai_addrlen);

    payload->total = ntohs(payload->total);
    for(int i = 0; i < MAX_SIZE_DADOS_TELEMETRIA; i++){
        payload->dados[i].id_cidade = ntohs(payload->dados[i].id_cidade);
        payload->dados[i].status = ntohs(payload->dados[i].status);
    }
}

void send_ack(int sockfd, const struct addrinfo *ai, header_t *header, payload_ack_t* payload){
    payload->status = htons(payload->status);

    char buf[sizeof(header_t) + sizeof(payload_ack_t)];
    memcpy(buf, header, sizeof(header_t));
    memcpy(buf + sizeof(header), payload, sizeof(payload_ack_t));

    sendto(sockfd, buf, strlen(buf), 0, ai->ai_addr, ai->ai_addrlen);

    payload->status = ntohs(payload->status);
}

void send_drone(int sockfd, const struct addrinfo *ai, header_t *header, payload_equipe_drone_t* payload){
    payload->id_cidade = htons(payload->id_cidade);
    payload->id_equipe = htons(payload->id_equipe);

    char buf[sizeof(header_t) + sizeof(payload_equipe_drone_t)];
    memcpy(buf, header, sizeof(header_t));
    memcpy(buf + sizeof(header), payload, sizeof(payload_equipe_drone_t));

    sendto(sockfd, buf, strlen(buf), 0, ai->ai_addr, ai->ai_addrlen);

    payload->id_cidade = ntohs(payload->id_cidade);
    payload->id_equipe = ntohs(payload->id_equipe);
}

void send_conclusao(int sockfd, const struct addrinfo *ai, header_t *header, payload_conclusao_t* payload){
    payload->id_cidade = htons(payload->id_cidade);
    payload->id_equipe = htons(payload->id_equipe);

    char buf[sizeof(header_t) + sizeof(payload_conclusao_t)];
    memcpy(buf, header, sizeof(header_t));
    memcpy(buf + sizeof(header), payload, sizeof(payload_conclusao_t));

    sendto(sockfd, buf, strlen(buf), 0, ai->ai_addr, ai->ai_addrlen);

    payload->id_cidade = ntohs(payload->id_cidade);
    payload->id_equipe = ntohs(payload->id_equipe);
}
