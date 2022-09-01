/*
Essential Vertices based Examination - head file
2022-02-27
------------------------------
C++ 11 
*/ 

#ifndef EVE_H
#define EVE_H
#include "../GraphUtils/Graph.cc"
#include "Verification/Verification.cc"



class EVE {

    public:

        EVE(Graph* inputGraph);
        EdgeID executeQuery(VertexID source, VertexID target, short k);                     // execute for each query
        void answerAllQueries(vector<PerQuery>& queries);                                   // experiments for answering all queries
        void cleanUp();                                                                     // free memory after running all queries

        // return space cost of current query for statistics file
        #ifdef WRITE_STATISTICS
            double getCurrentSpaceCost();                                                   
        #endif

    private:

        // basic graph information
        Graph* graph;
        VertexID VN;                                                                        // |V| of graph
        EdgeID EN;                                                                          // |E| of graph
        PerEdge* edges;                                                                     // store all edges in graph
        PerNeighbor *outNeighbors, *inNeighbors;                                            // neighbors of each vertex, length=EN
        EdgeID *outNeighborsLocator, *inNeighborsLocator;                                   // locate where to find the neighbors of a vertex, length=VN

        // initialize and refresh memory for queries
        void initEVE(); 
        inline void refreshMemory();
        
        // for storing neighbors of upper-bound graph
        int *hasPrunedInNeighbors, *hasPrunedOutNeighbors;
        PerNeighbor *prunedInNeighbors, *prunedOutNeighbors;
        EdgeID *prunedInNeighborsEnd, *prunedOutNeighborsEnd;

        // adaptive bi-directional BFS
        VertexID s, t, forwardMinId, forwardMaxId, backwardMinId, backwardMaxId;
        void adaptiveBiDirectBFS();
        bool continueDirection, startPropDirection;                                         // true for forward, false for backward

        // Propagation for calculating essential vertices
        VertexID *forwardEV0, *backwardEV0;                                                 // store essential vertices set, length=(maxLen-2)*VN*(maxLen-2)
        inline void addToEV(VertexID u, VertexID* EVStart, int EVLen);                      // add a vertex u to essential vertices set
        int *forwardEVLen0, *backwardEVLen0;                                                // essential vertices set end-position, length=(maxLen-2)*VN
        int *forwardLastLocation, *backwardLastLocation;                                    // last valid k for EV_k
        VertexID *lastEV;
        int *lastEVEnds;
        void forwardPropagation();                          
        void backwardPropagation();                       

        // edge candidates found in search
        EdgeID *candidates, candidateEnd;                                                   
        inline void addToFinalCandidates(EdgeID& edgeId);

        // intersaction
        int* isInResult;
        EdgeID *results, resultEnd;                                                         // results of edge ids, length=EN
        short edgeLabeling(EdgeID& edgeId);                                                 // intersact for each candidate edge, return 1 if candidates, 2 if in result

        // departures and arrivals
        VertexID *InD, *OutA, *departures, departuresEnd, *arrivals, arrivalsEnd;

        // verify undetermined edges
        inline void addToPrunedNeighbors(VertexID& u, VertexID& v, EdgeID& edgeId);
        Verification* verification;
        EdgeID *edgesForVerification, edgesForVerificationEnd;
        VertexID *verticesHavingOutNeighbors, *verticesHavingInNeighbors, verticesHavingOutNeighborsEnd, verticesHavingInNeighborsEnd;
 
        // write statistics of queries to file    
        #ifdef WRITE_STATISTICS
            void initStatisticStorage(int queryNumber);
            void cleanUpStatisticStorage();  
            int queryId=0;
            EdgeID *numOfUpperbound, *numOfAnswers, specialCnt;        
            VertexID prunedNeighborsCount, InDoutACount, forwardEVCount, backwardEVCount, maxFrontierSize;   
            double *spaceCosts;  
        #endif                     
};



// storage of essential vertices for simplicity
#define forwardEV(i,j) (forwardEV0[((i)-1)*VN*(maxLen-2)+(j)*(maxLen-2)])                   // forwardEV(k,u):      the start of EV_k(s,u)
#define backwardEV(i,j) (backwardEV0[((i)-1)*VN*(maxLen-2)+(j)*(maxLen-2)])                 // backwardEV(k,u):     the start of EV_k(v,t)
#define forwardEVLen(i,j) (forwardEVLen0[((i)-1)*VN+(j)])                                   // forwardEVLen(i,j):   length of EV_k(s,u)
#define backwardEVLen(i,j) (backwardEVLen0[((i)-1)*VN+(j)])                                 // backwardEVLen(i,j):  length of EV_k(v,t)



#endif