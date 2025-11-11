#include "utils.h"

int findNearestAvailableDroneCrew(Graph* graph, int src){
    int dist[graph->num_vertices], min_dist = INT_MAX, min_dist_id = -1;

    dijkstra(graph, src, dist);

    // find the nearest capital city and return its id
    for (int i = 0; i < graph->num_vertices; i++) {
        if(i == src) continue;
        if(graph->vertices[i].type == 0) continue; // not capital
        if(dist[i] < min_dist){
            min_dist = dist[i];
            min_dist_id = i;
        }
    }

    return min_dist_id;
}