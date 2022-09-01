/*
GraphUtils - implementations
2021-09-21
------------------------------
C++ 11 
*/ 

#ifndef GRAPH_CC
#define GRAPH_CC
#include "Graph.h"
using namespace std;


Graph::Graph(const char* inputGraphFilename) {
    graphFilename = inputGraphFilename;
    loadGraphFile();
}


void Graph::loadGraphFile() {

    double startTime = getCurrentTimeInMs();
    printf("Loading graph file: %s ...\n", graphFilename);
    
    // open file, read in |V| and |E|
    freopen(graphFilename, "r", stdin);
    scanf("%d%d", &VN, &EN);

    // initialize arrays for storing neighbors
    inNeighbors = new PerNeighbor[EN];
    inNeighborsLocator = new EdgeID[VN+1];
    outNeighbors = new PerNeighbor[EN];
    outNeighborsLocator = new EdgeID[VN+1];

    // scanf the graph file line by line
    VertexID fromId, toId;
    edges = new PerEdge[EN];
    EdgeID i = 0;
    while (i<EN) {
        scanf("%d,%d", &fromId, &toId);
        edges[i] = {i, fromId, toId};
        i++;
    }        
    
    // store in-neighbors
    sort(edges, edges+EN, sortByToId);
    EdgeID curLocator = 0;
    VertexID curId = 0, id;
    i = 0;
    while (i<EN) {
        id = edges[i].toId;
        while (curId<=id) {
            inNeighborsLocator[curId] = curLocator;
            curId++;
        }
        while (i<EN && edges[i].toId==id) {
            inNeighbors[curLocator] = {edges[i].edgeId, edges[i].fromId};
            curLocator++;
            i++;
        }
    }
    while (curId<=VN) {
        inNeighborsLocator[curId] = curLocator;
        curId++;
    }

    // store out-neighbors
    sort(edges, edges+EN, sortByFromId);
    curLocator = 0;
    curId = 0;
    i = 0;
    while (i<EN) {
        id = edges[i].fromId;
        while (curId<=id) {
            outNeighborsLocator[curId] = curLocator;
            curId++;
        }
        while (i<EN && edges[i].fromId==id) {
            outNeighbors[curLocator] = {edges[i].edgeId, edges[i].toId};
            curLocator++;
            i++;
        }
    }
    while (curId<=VN) {
        outNeighborsLocator[curId] = curLocator;
        curId++;
    }

    // sort by edge id
    sort(edges, edges+EN, sortByEdgeId);
    
    double timeCost = getCurrentTimeInMs() - startTime;
    printf("- Finish. |V|=%d and |E|=%d, time cost: %.2f ms\n", VN, EN, timeCost);
    logFile<<VN<<","<<EN<<","<<str(timeCost)<<",";
}

#endif