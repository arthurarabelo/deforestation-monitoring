#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CITY_NAME 50

int main(){
    FILE *file = fopen("grafo_random.txt", "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    int N, M;
    fscanf(file, "%d %d", &N, &M);

    Graph* forest_graph = createGraph(N);

    // read vertices
    for (int i = 0; i < N; i++) {
        int city_id, city_type;
        char city_name[MAX_CITY_NAME];
        fscanf(file, "%d %s %d", &city_id, city_name, &city_type);
        setVertexInfo(forest_graph, i, city_name, city_type, city_id);
    }

    // read edges
    for (int i = 0; i < M; i++) {
        int u, v, weight;
        fscanf(file, "%d %d %d", &u, &v, &weight);
        addEdge(forest_graph, u, v, weight);
    }

    fclose(file);

    

    freeGraph(forest_graph);

    return 0;
}