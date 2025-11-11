#include "graph.h"

// allocate a graph with n vertices
Graph* createGraph(int num_vertices) {
    Graph* graph = malloc(sizeof(Graph));
    graph->num_vertices = num_vertices;
    graph->vertices = calloc(num_vertices, sizeof(Vertex)); // initialize adjacency lists to NULL
    return graph;
}

void setVertexInfo(Graph* graph, int vertex_index, char* city_name, int city_type, int city_id){
    graph->vertices[vertex_index].name = city_name;
    graph->vertices[vertex_index].type = city_type;
    graph->vertices[vertex_index].id_cidade = city_id;
}

// add an edge (u -> v) with given weight
void addEdge(Graph* graph, int src, int dest, int weight) {
    Edge* newEdge = malloc(sizeof(Edge));
    newEdge->dest = dest;
    newEdge->weight = weight;
    newEdge->next = graph->vertices[src].head;
    graph->vertices[src].head = newEdge;
}

int findMinDistance(int* dist, int* included, int num_vertices) {
    int min = INT_MAX, min_index;

    // Traverse all vertices to find the vertex with the
    // minimum distance value
    for (int v = 0; v < num_vertices; v++) {
        if (included[v] == 0 && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

// check if vertex u is connected with vertex v
// if true, returns the weight of the connection
int checkIfThereIsAConnection(Graph* graph, int u, int v){
    Edge* edge = graph->vertices[u].head;
    while (edge) {
        if(edge->dest == v) return edge->weight;
        edge = edge->next;
    }

    return 0;
}

void dijkstra(Graph* graph, int src, int *dist){
    // Array to keep track of included nodes
    int included[graph->num_vertices];

    // Initialize all distances as INFINITE and included as
    // false
    for (int i = 0; i < graph->num_vertices; i++) {
        dist[i] = INT_MAX;
        included[i] = 0;
    }

    // Distance of source vertex from itself is always 0
    dist[src] = 0;

    // Find the shortest path for all vertices
    for (int count = 0; count < graph->num_vertices - 1; count++) {
        // Pick the minimum distance vertex from the set of
        // vertices not yet processed
        int u = findMinDistance(dist, included, graph->num_vertices);

        // Mark the selected vertex as included
        included[u] = 1;

        // Update the distance of all the adjacent vertices
        // of the selected vertex
        for (int v = 0; v < graph->num_vertices; v++) {
            // update dist[v] if it is already note included
            // and the current distance is less then it's
            // original distance
            int dist_u_v = checkIfThereIsAConnection(graph, u, v);

            if (!included[v] && dist_u_v
                && dist[u] != INT_MAX
                && dist[u] + dist_u_v < dist[v]) {
                dist[v] = dist[u] + dist_u_v;
            }
        }
    }
}

// free memory allocated to graph
void freeGraph(Graph* graph) {
    for (int i = 0; i < graph->num_vertices; i++) {
        Edge* edge = graph->vertices[i].head;
        while (edge) {
            Edge* temp = edge;
            edge = edge->next;
            free(temp);
        }
    }
    free(graph->vertices);
    free(graph);
}