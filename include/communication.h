#include <stdint.h>

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
    telemetria_t dados[50]; // list of (id_cidade, status)
} payload_telemetria_t;

typedef struct {
    int status; // 0 = ACK_TELEMETRIA, 1 = ACK_EQUIPE DRONE, 2 = ACK_CONCLUSAO
} payload_ack_t;

typedef struct {
    int id_cidade; // city where the warning was detected
    int id_equipe; // designated drone crew
} payload_equipe_drone_t;

typedef struct {
    int id_cidade; // city that was helped
    int id_equipe; // crew who helped
} payload_conclusao_t;

