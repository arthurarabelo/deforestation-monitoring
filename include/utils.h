#ifndef UTILS_H
#define UTILS_H

#include "communication.h"
#include "graph.h"
#include "alertQueue.h"

int findNearestAvailableDroneCrew(Graph* graph, int src, int *dist);

void handle_telemetry(payload_telemetria_t *payload, alert_queue_t *alerts);

#endif