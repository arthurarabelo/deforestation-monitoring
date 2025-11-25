#ifndef COMUNICATION_H
#define COMUNICATION_H

#define _GNU_SOURCE
#define MAX_SIZE_DADOS_TELEMETRIA 50

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
struct addrinfo;
struct sockaddr;

typedef enum {
    MSG_TELEMETRIA = 1,
    MSG_ACK = 2,
    MSG_EQUIPE_DRONE = 3,
    MSG_CONCLUSAO = 4
} MessageType;

typedef struct {
    uint16_t tipo; // msg type
    uint16_t tamanho; // payload size
} header_t;

typedef struct {
    int id_cidade; // vertex identifier
    int status; // 0 = OK, 1 = WARNING
} telemetria_t;

typedef struct {
    int total; // number of monitored cities
    telemetria_t dados[MAX_SIZE_DADOS_TELEMETRIA]; // list of (id_cidade, status)
} payload_telemetria_t;

typedef struct {
    int status; // 0 = ACK_TELEMETRIA, 1 = ACK_EQUIPE_DRONE, 2 = ACK_CONCLUSAO
} payload_ack_t;

typedef struct {
    int id_cidade; // city where the warning was detected
    int id_equipe; // designated drone crew
} payload_equipe_drone_t;

typedef struct {
    int id_cidade; // city that was helped
    int id_equipe; // crew who helped
} payload_conclusao_t;

typedef struct {
    int ack; // 0 = packages were lost, 1 = message received completely
    header_t h;
    payload_telemetria_t p_telemetry;
    payload_ack_t p_ack;
    payload_equipe_drone_t p_drone;
    payload_conclusao_t p_conclusion;
} answer_t;

#define MAX_BUFFER_SIZE (sizeof(header_t) + sizeof(payload_telemetria_t))

void *get_in_addr(struct sockaddr *sa);

void host_to_network_short_header(header_t *header);
void network_to_host_short_header(header_t *header);

void host_to_network_long_payload(void *payload, MessageType type);
void network_to_host_long_payload(void *payload, MessageType type);

void send_message(int sockfd, struct addrinfo *p, MessageType tipo, void *payload, size_t payload_size);

void receive_message(int sockfd, struct sockaddr_in *their_addr, socklen_t *addrlen, answer_t *answer);

#endif