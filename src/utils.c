#include "utils.h"

int findNearestAvailableDroneCrew(Graph* graph, int src, int *dist){
    int min_dist = INT_MAX, min_dist_id = -1;

    dijkstra(graph, src, dist);

    // find the nearest capital city and return its id
    for (int i = 0; i < graph->num_vertices; i++) {
        if(graph->vertices[i].type == 0) continue; // not capital
        if(graph->vertices[i].available == 0) continue; // not available
        
        if(dist[i] < min_dist){
            min_dist = dist[i];
            min_dist_id = i;
        }
    }

    return min_dist_id;
}

void handle_telemetry(payload_telemetria_t *payload, alert_queue_t *alerts){
    for(int i = 0; i < payload->total; i++){
        if(payload->dados[i].status == 1){
            /* ALERTA */
            if(!has_alert_element(alerts, i)){
                enqueue_alert(alerts, i, 0, time(NULL));
            }
        }
    }
}