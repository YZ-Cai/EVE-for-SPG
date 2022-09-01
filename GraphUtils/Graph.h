/*
GraphUtils - head file
2021-09-21
------------------------------
C++ 11 
*/ 

#ifndef GRAPH_H
#define GRAPH_H
#include "Utils.h"


// storing each edge
struct PerEdge {
    EdgeID edgeId;
    VertexID fromId, toId;
};


// storing in- or out-neighbors
struct PerNeighbor {
    EdgeID edgeId;
    VertexID neighbor;
    PerNeighbor(): edgeId(0), neighbor(0) {}
    PerNeighbor(EdgeID a, VertexID b): edgeId(a), neighbor(b) {}
    bool operator<(const PerNeighbor& a)const {
        return neighbor<a.neighbor;
    }
};



class Graph {

    public:

        // basic graph infomation
        Graph(const char* inputGraphFilename);
        VertexID VN;                                                // |V| of graph
        EdgeID EN;                                                  // |E| of graph
        PerEdge* edges;                                             // store all edges in graph
        PerNeighbor *inNeighbors, *outNeighbors;                    // neighbors of each vertex, length=EN
        EdgeID *inNeighborsLocator, *outNeighborsLocator;           // locate where to find the neighbors of a vertex, length=VN
        
        /*
        Compressed Sparse Row (CSR)
        If V={0,1,2,3} and E={0->1, 0->2, 0->3, 1->2, 1->3, 2->3} then:
        outNeighbors = [1,2,3,2,3,3] and outNeighborsLocator = [0,3,5],
        which means the out-neighbors of vertex v are: outNeighbors[outNeighborsLocator[v]:outNeighborsLocator[v+1]].
        For example, the out-neighbors of vertex 0 are: outNeighbor[0:3], i.e., [1,2,3]
        */

    private:

        // load graph file
        const char* graphFilename;
        void loadGraphFile();

};



// for sorting edges

bool sortByFromId(PerEdge& a, PerEdge& b) {
    if (a.fromId==b.fromId)
        return a.toId<b.toId;
    return a.fromId<b.fromId;
}

bool sortByToId(PerEdge& a, PerEdge& b) {
    if (a.toId==b.toId)
        return a.fromId<b.fromId;
    return a.toId<b.toId;
}

bool sortByEdgeId(PerEdge& a, PerEdge& b) {
    return a.edgeId<b.edgeId;
}



#endif