#ifndef GRAPH_H
#define GRAPH_H

#define MAX_CITY_NAME 50
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

typedef struct edge {
    int dest;
    int weight;
    struct edge* next;
} Edge;

typedef struct vertex {
    char name[MAX_CITY_NAME]; // name of the city
    int type; // 0 = regional, 1 = capital
    int id_cidade; // identificador da cidade
    int available; // 0 = unavailable, 1 = available
    Edge* head;
} Vertex;

typedef struct graph {
    int num_vertices;
    Vertex* vertices;
} Graph;

Graph* createGraph(int numVertices);

void addEdge(Graph* graph, int src, int dest, int weight);

void setVertexInfo(Graph* graph, int vertex_index, char* city_name, int city_type, int city_id);

void freeGraph(Graph* graph);

void dijkstra(Graph* graph, int src, int* dist);

int checkIfThereIsAConnection(Graph* graph, int u, int v);

int findMinDistance(int* dist, int* included, int num_vertices);

#endif
