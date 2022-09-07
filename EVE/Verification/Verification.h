#ifndef VERIFICATION_H
#define VERIFICATION_H
#include "../../GraphUtils/Graph.cc"



class Verification {

    public:

        Verification(Graph* inputGraph, VertexID* inputResults, int* inputIsInResult);
        VertexID verifyUndeterminedEdge(VertexID& inputResultEnd, EdgeID& inputEdgesForVerificationEnd, 
                                        VertexID& inputDeparturesEnd, VertexID& inputArrivalsEnd, 
                                        VertexID& inputVerticesHavingOutNeighborsEnd, VertexID& inputVerticesHavingInNeighborsEnd);
        double getCurrentSpaceCost();

        // results
        int* isInResult;
        VertexID resultEnd;

        // refresh and clean up memories
        void refreshMemory();
        void cleanUp();

        // departures, arrivals and their neighbors
        VertexID *InD, *OutA, *departures, departuresEnd, *arrivals, arrivalsEnd, departure, arrival;

        // for storing neighbors in upper-bound graph
        int *hasPrunedInNeighbors, *hasPrunedOutNeighbors;
        PerNeighbor *prunedInNeighbors, *prunedOutNeighbors;
        EdgeID *prunedInNeighborsEnd, *prunedOutNeighborsEnd;

        // all edges for verification
        EdgeID *edgesForVerification, edgesForVerificationEnd;
        
        // vertices having pruned in or out neighbors
        VertexID *verticesHavingOutNeighbors, *verticesHavingInNeighbors, verticesHavingOutNeighborsEnd, verticesHavingInNeighborsEnd;

    private:

        // basic graph information
        Graph* graph;
        VertexID VN;                                                        // |V| of graph
        EdgeID EN;                                                          // |E| of graph
        PerEdge* edges;                                                     // store all edges in graph
        PerNeighbor *outNeighbors, *inNeighbors;                            // neighbors of each vertex, length=EN
        EdgeID *outNeighborsLocator, *inNeighborsLocator;                   // locate where to find the neighbors of a vertex, length=VN

        // BFS search from departures and arrivals and sort pruned neighbors
        VertexID *frontier, frontierEnd;
        bool useSearchOrderingStrategy;
        void BFS();
        void reOrderingNeighbors();

        // DFS search
        EdgeID* curPath;                                                    // record edges in current DFS path
        VertexID curFromId, curToId, curPathEnd=0;
        VertexID *results, *InC, *OutC;
        
        bool *inStack;
        bool forwardSearch(VertexID& u);
        bool backwardSearch(VertexID& u);
        bool forwardFinalSearch(VertexID& u);
        bool backwardFinalSearch(VertexID& u);
        bool tryAddEdges();
        void addToResults();

        // initialization
        void initVerification();
};



// threshold for search ordering strategy
EdgeID sortPrunedThreshold = 1024;



// for sorting neighbors (search ordering strategy)
short *InDEnds, *OutAEnds;
int *isDeparture, *isArrival, *forwardVisited, *backwardVisited;

bool sortByDepartures(PerNeighbor& a, PerNeighbor& b) {
    if (forwardVisited[a.neighbor]==offset && forwardVisited[b.neighbor]==offset) {
        if (forwardDist[a.neighbor]==0 && forwardDist[b.neighbor]==0)
            return InDEnds[a.neighbor]>InDEnds[b.neighbor];
        return forwardDist[a.neighbor]<forwardDist[b.neighbor];
    }
    return forwardVisited[a.neighbor]==offset;
}

bool sortByArrivals(PerNeighbor& a, PerNeighbor& b) {
    if (backwardVisited[a.neighbor]==offset && backwardVisited[b.neighbor]==offset) {
        if (backwardDist[a.neighbor]==0 && backwardDist[b.neighbor]==0)
            return OutAEnds[a.neighbor]>OutAEnds[b.neighbor];
        return backwardDist[a.neighbor]<backwardDist[b.neighbor];
    }
    return backwardVisited[a.neighbor]==offset;
}



#endif