/*
Verifying Undetermined Edges - implementations
Author: Yuzheng Cai
2022-03-01
------------------------------
C++ 11 
*/ 

#ifndef VERIFICATION_CC
#define VERIFICATION_CC
#include "Verification.h"
using namespace std;



Verification::Verification(Graph* inputGraph, VertexID* inputResults, int* inputIsInResult) {

    // basic graph information
    graph = inputGraph;
    VN = graph->VN;
    EN = graph->EN;
    edges = graph->edges;
    inNeighborsLocator = graph->inNeighborsLocator;
    inNeighbors = graph->inNeighbors;
    outNeighborsLocator = graph->outNeighborsLocator;
    outNeighbors = graph->outNeighbors;

    // initialization
    results = inputResults;
    isInResult = inputIsInResult;
    initVerification();
}



// verification for undetermined edges 
VertexID Verification::verifyUndeterminedEdge(VertexID& inputResultEnd, EdgeID& inputEdgesForVerificationEnd, 
                                              VertexID& inputVerticesHavingOutNeighborsEnd, VertexID& inputVerticesHavingInNeighborsEnd, 
                                              VertexID& inputDeparturesEnd, VertexID& inputArrivalsEnd) {
    
    // obtain information from upper-bound graph
    resultEnd = inputResultEnd;
    edgesForVerificationEnd = inputEdgesForVerificationEnd;
    verticesHavingOutNeighborsEnd = inputVerticesHavingOutNeighborsEnd;
    verticesHavingInNeighborsEnd = inputVerticesHavingInNeighborsEnd;
    departuresEnd = inputDeparturesEnd;
    arrivalsEnd = inputArrivalsEnd;

    // apply search ordering strategry
    if (maxLen>6 && edgesForVerificationEnd>sortPrunedThreshold) {
        useSearchOrderingStrategy = true;
        BFS();
        reOrderingNeighbors();
    } else 
        useSearchOrderingStrategy = false;

    // iterate each undetermined edges
    for (EdgeID i=0; i<edgesForVerificationEnd; i++) {
        EdgeID& edgeId = edgesForVerification[i];
        if (isInResult[edgeId]==offset)
            continue;

        // initialization for DFS
        curFromId = edges[edgeId].fromId;
        curToId = edges[edgeId].toId;
        curPath[0] = edgeId;
        curPathEnd = 1;
        inStack[curToId] = true;
        inStack[curFromId] = true;
        
        // if current edge links a departure
        if (isDeparture[curFromId]==offset) {
            departure = curFromId;

            // if it also links an arrival
            if (isArrival[curToId]==offset) {
                arrival = curToId;
                if (tryAddEdges()) {
                    inStack[curToId] = false;
                    inStack[curFromId] = false;
                    continue;
                }
            }

            // if it does not link an arrival, search forward
            if (maxLen>5 && forwardFinalSearch(curToId)) {
                inStack[curToId] = false;
                inStack[curFromId] = false;
                continue;
            }
        }

        // if current edge links an arrival, search backward
        if (maxLen>5 && isArrival[curToId]==offset) {
            arrival = curToId;
            if (backwardFinalSearch(curFromId)) {
                inStack[curToId] = false;
                inStack[curFromId] = false;
                continue;
            }
        }
        
        // decide to search forward or backward first by number of neighbors in each direction
        if (maxLen>6) {
            if (prunedOutNeighborsEnd[curToId]-outNeighborsLocator[curToId]<=prunedInNeighborsEnd[curFromId]-inNeighborsLocator[curFromId])
                forwardSearch(curToId);
            else    
                backwardSearch(curFromId);
        }
        inStack[curToId] = false;
        inStack[curFromId] = false;
    }
    return resultEnd;
}



// initialization
void Verification::initVerification() {

    // neighbors for upper-bound graph
    prunedOutNeighbors = new PerNeighbor[EN];
    prunedInNeighbors = new PerNeighbor[EN];
    prunedOutNeighborsEnd = new EdgeID[VN];
    prunedInNeighborsEnd = new EdgeID[VN];
    hasPrunedOutNeighbors = new int[VN]();
    hasPrunedInNeighbors = new int[VN]();

    // departures, arrivals and their neighbors
    departures = new VertexID[VN];
    arrivals = new VertexID[VN];
    InD = new VertexID[VN*(maxLen-2)];
    OutA = new VertexID[VN*(maxLen-2)];
    InDEnds = new short[VN];
    OutAEnds = new short[VN];
    isDeparture = new int[VN]();
    isArrival = new int[VN]();

    // edges to be verified
    edgesForVerification = new EdgeID[EN];

    // DFS search
    inStack = new bool[VN]();
    curPath = new EdgeID[maxLen-4];
    InC = new VertexID[2];
    OutC = new VertexID[2];

    // for search ordering strategy
    if (maxLen>6) {
        verticesHavingInNeighbors = new VertexID[VN];
        verticesHavingOutNeighbors = new VertexID[VN];
        forwardVisited = new int[VN]();
        backwardVisited = new int[VN]();
    }
}



// return space costs of verification
double Verification::getCurrentSpaceCost() {
    double spaceCost = 0;

    // neighbors for upper-bound graph
    spaceCost += 0;                                                     // prunedOutNeighbors will be calculated in EVE.getCurrentSpaceCost()
    spaceCost += 0;                                                     // prunedInNeighbors will be calculated in EVE.getCurrentSpaceCost()
    spaceCost += sizeof(EdgeID)*VN;                                     // prunedOutNeighborsEnd = new EdgeID[VN];
    spaceCost += sizeof(EdgeID)*VN;                                     // prunedInNeighborsEnd = new EdgeID[VN];
    spaceCost += sizeof(int)*VN;                                        // hasPrunedOutNeighbors = new int[VN]();
    spaceCost += sizeof(int)*VN;                                        // hasPrunedInNeighbors = new int[VN]();

    // departures, arrivals and their neighbors
    spaceCost += sizeof(VertexID)*departuresEnd;                        // departures = new VertexID[VN];
    spaceCost += sizeof(VertexID)*arrivalsEnd;                          // arrivals = new VertexID[VN];
    spaceCost += 0;                                                     // InD will be calculated in EVE.getCurrentSpaceCost()
    spaceCost += 0;                                                     // OutA will be calculated in EVE.getCurrentSpaceCost()
    spaceCost += sizeof(short)*VN;                                      // InDEnds = new short[VN];
    spaceCost += sizeof(short)*VN;                                      // OutAEnds = new short[VN];
    spaceCost += sizeof(int)*VN;                                        // isDeparture = new int[VN]();
    spaceCost += sizeof(int)*VN;                                        // isArrival = new int[VN]();

    // edges to be verified
    spaceCost += sizeof(EdgeID)*edgesForVerificationEnd;                // edgesForVerification = new EdgeID[EN];

    // DFS search
    spaceCost += sizeof(bool)*VN;                                       // inStack = new bool[VN]();
    spaceCost += sizeof(EdgeID)*(maxLen-4);                             // curPath = new EdgeID[maxLen-4];
    spaceCost += sizeof(VertexID)*2;                                    // InC = new VertexID[2];
    spaceCost += sizeof(VertexID)*2;                                    // OutC = new VertexID[2];

    // for search ordering strategy
    if (maxLen>6) {
        spaceCost += sizeof(VertexID)*verticesHavingInNeighborsEnd;     // verticesHavingInNeighbors = new VertexID[VN];
        spaceCost += sizeof(VertexID)*verticesHavingOutNeighborsEnd;    // verticesHavingOutNeighbors = new VertexID[VN];
        spaceCost += sizeof(int)*VN;                                    // forwardVisited = new int[VN]();
        spaceCost += sizeof(int)*VN;                                    // backwardVisited = new int[VN]();
    }

    return spaceCost;
}



// refresh memory for new query
void Verification::refreshMemory() {
    memset(isDeparture, 0, sizeof(int)*VN);
    memset(isArrival, 0, sizeof(int)*VN);
    memset(hasPrunedOutNeighbors, 0, sizeof(int)*VN);
    memset(hasPrunedInNeighbors, 0, sizeof(int)*VN);
    if (maxLen>6) {
        memset(forwardVisited, 0, sizeof(int)*VN);
        memset(backwardVisited, 0, sizeof(int)*VN);
    }
}



// clean up memories allocated
void Verification::cleanUp() {
    delete[] prunedOutNeighbors;
    delete[] prunedInNeighbors;
    delete[] prunedOutNeighborsEnd;
    delete[] prunedInNeighborsEnd;
    delete[] hasPrunedOutNeighbors;
    delete[] hasPrunedInNeighbors;
    delete[] departures;
    delete[] arrivals;
    delete[] InD;
    delete[] OutA;
    delete[] InDEnds;
    delete[] OutAEnds;
    delete[] isDeparture;
    delete[] isArrival;
    delete[] edgesForVerification;
    if (maxLen>6) {
        delete[] verticesHavingInNeighbors;
        delete[] verticesHavingOutNeighbors;
        delete[] forwardVisited;
        delete[] backwardVisited;
    }
    delete[] inStack;
    delete[] curPath;
    delete[] InC;
    delete[] OutC;
}



// BFS search from departures and arrivals
void Verification::BFS() {

    // initialization
    frontier = forwardFrontier;

    // forward BFS from departures
    frontierEnd = 0;
    for (VertexID i=0; i<departuresEnd; i++) {
        VertexID& u = departures[i];
        forwardDist[u] = 0;
        forwardVisited[u] = offset;
        frontier[frontierEnd] = u;
        frontierEnd++;
    }
    
    // BFS search from 1 to maxLen-5
    for (short k=1; k<=maxLen-5; k++) {
        nextFrontierEnd = 0;
        for (VertexID i=0; i<frontierEnd; i++) {
            VertexID& u = frontier[i];
            if (hasPrunedOutNeighbors[u]==offset)
                for (EdgeID outEdgeLocation=outNeighborsLocator[u]; outEdgeLocation<prunedOutNeighborsEnd[u]; outEdgeLocation++) {
                    VertexID& v = prunedOutNeighbors[outEdgeLocation].neighbor;

                    // record distance from departures, push to next frontier
                    if (forwardVisited[v]<offset) {
                        forwardVisited[v] = offset;
                        forwardDist[v] = k;
                        if (k<maxLen-5) {
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;
                        }
                    }
                }
        }

        // swap frontier for next step
        if (k<maxLen-5) {
            VertexID* tmp = frontier;
            frontier = nextFrontier;
            nextFrontier = tmp;
            frontierEnd = nextFrontierEnd;
        }
    }

    // backward BFS from arrivals
    frontierEnd = 0;
    for (VertexID i=0; i<arrivalsEnd; i++) {
        VertexID& u = arrivals[i];
        backwardDist[u] = 0;
        backwardVisited[u] = offset;
        frontier[frontierEnd] = u;
        frontierEnd++;
    }
    
    // BFS search from 1 to maxLen-5
    for (short k=1; k<=maxLen-5; k++) {
        nextFrontierEnd = 0;
        for (VertexID i=0; i<frontierEnd; i++) {
            VertexID& u = frontier[i];
            if (hasPrunedInNeighbors[u]==offset)
                for (EdgeID inEdgeLocation=inNeighborsLocator[u]; inEdgeLocation<prunedInNeighborsEnd[u]; inEdgeLocation++) {
                    VertexID& v = prunedInNeighbors[inEdgeLocation].neighbor;

                    // record distance to arrivals, push to next frontier
                    if (backwardVisited[v]<offset) {
                        backwardVisited[v] = offset;
                        backwardDist[v] = k;
                        if (k<maxLen-5) {
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;
                        }
                    }
                }
        }

        // swap frontier for next step
        if (k<maxLen-5) {
            VertexID* tmp = frontier;
            frontier = nextFrontier;
            nextFrontier = tmp;
            frontierEnd = nextFrontierEnd;
        }
    }
}



// for sorting neighbors (search ordering strategy)
void Verification::reOrderingNeighbors() {

    // sort pruned out neighbors
    for (VertexID i=0; i<verticesHavingOutNeighborsEnd; i++) {
        VertexID& u = verticesHavingOutNeighbors[i];
        EdgeID& uStart = outNeighborsLocator[u];
        EdgeID& uEnd = prunedOutNeighborsEnd[u];
        sort(prunedOutNeighbors+uStart, prunedOutNeighbors+uEnd, sortByArrivals);

        // remove edges can not reach any arrivals
        while (uEnd>uStart) {
            if (backwardVisited[prunedOutNeighbors[uEnd-1].neighbor]==offset)
                break;
            uEnd--;
        }
    }

    // sort pruned in neighbors
    for (VertexID i=0; i<verticesHavingInNeighborsEnd; i++) {
        VertexID& u = verticesHavingInNeighbors[i];
        EdgeID& uStart = inNeighborsLocator[u];
        EdgeID& uEnd = prunedInNeighborsEnd[u];
        sort(prunedInNeighbors+uStart, prunedInNeighbors+uEnd, sortByDepartures);

        // remove edges can not be reached by any departures
        while (uEnd>uStart) {
            if (forwardVisited[prunedInNeighbors[uEnd-1].neighbor]==offset)
                break;
            uEnd--;
        }
    }    
}



// search forward when no departure specified
bool Verification::forwardSearch(VertexID& u) {
    bool ans = false;

    // iterate each out edge u->v
    if (hasPrunedOutNeighbors[u]==offset)
        for (EdgeID outEdgeLocation=outNeighborsLocator[u]; outEdgeLocation<prunedOutNeighborsEnd[u]; outEdgeLocation++) {
            VertexID& v = prunedOutNeighbors[outEdgeLocation].neighbor;
            if (useSearchOrderingStrategy && curPathEnd+1+backwardDist[v]+1>maxLen-4)
                break;

            // if next node not in stack
            if (inStack[v]==false) {
                EdgeID& edgeId = prunedOutNeighbors[outEdgeLocation].edgeId; 
                curPath[curPathEnd] = edgeId;
                curPathEnd++;
                inStack[v] = true;

                // if reach an arrival, start search backward
                if (isArrival[v]==offset) {
                    arrival = v;
                    if (backwardFinalSearch(curFromId))
                        ans = true;
                }
                
                // continue forward search
                if (ans==false && curPathEnd+2<=maxLen-4 && forwardSearch(v)) 
                    ans = true;

                // pop edge e(u,v) and vertex v from stacks
                curPathEnd--;
                inStack[v] = false;
                if (ans)
                    return true;                
            }
        }
    return false;
}



// search backward given specified arrival
bool Verification::backwardFinalSearch(VertexID& u) {
    bool ans = false;

    // iterate each in edge v->u
    if (hasPrunedInNeighbors[u]==offset)
        for (EdgeID inEdgeLocation=inNeighborsLocator[u]; inEdgeLocation<prunedInNeighborsEnd[u]; inEdgeLocation++) {
            VertexID& v = prunedInNeighbors[inEdgeLocation].neighbor;
            if (useSearchOrderingStrategy && curPathEnd+1+forwardDist[v]>maxLen-4)
                break;
            
            // if next node not in stack
            if (inStack[v]==false) {
                EdgeID& edgeId = prunedInNeighbors[inEdgeLocation].edgeId; 
                curPath[curPathEnd] = edgeId;
                curPathEnd++;
                inStack[v] = true;

                // if reach a departure, try add edges in stack to results
                if (isDeparture[v]==offset) {
                    departure = v;
                    if (tryAddEdges())
                        ans = true;
                }
                
                // continue backward search
                if (ans==false && curPathEnd+1<=maxLen-4 && backwardFinalSearch(v))
                    ans = true;
                
                // pop edge e(v,u) and vertex v from stacks
                curPathEnd--;
                inStack[v] = false;
                if (ans)
                    return true;
            }
        }
    return false;
}



// search backward when no departure specified
bool Verification::backwardSearch(VertexID& u) {
    bool ans = false;

    // iterate each in edge v->u
    if (hasPrunedInNeighbors[u]==offset)
        for (EdgeID inEdgeLocation=inNeighborsLocator[u]; inEdgeLocation<prunedInNeighborsEnd[u]; inEdgeLocation++) {
            VertexID& v = prunedInNeighbors[inEdgeLocation].neighbor;
            if (useSearchOrderingStrategy && curPathEnd+1+forwardDist[v]+1>maxLen-4)
                break;
            
            // if next node not in stack
            if (inStack[v]==false) {
                EdgeID& edgeId = prunedInNeighbors[inEdgeLocation].edgeId; 
                curPath[curPathEnd] = edgeId;
                curPathEnd++;
                inStack[v] = true;

                // if reach a departure, start forward search
                if (isDeparture[v]==offset) {
                    departure = v;
                    if (forwardFinalSearch(curToId))
                        ans = true;
                }
                
                // continue backward search
                if (ans==false && curPathEnd+2<=maxLen-4 && backwardSearch(v))
                    ans = true;
                
                // pop edge e(v,u) and vertex v from stacks
                curPathEnd--;
                inStack[v] = false;
                if (ans)
                    return true;
            }
        }
    return false;
}



// search forward given specified departure
bool Verification::forwardFinalSearch(VertexID& u) {
    bool ans = false;

    // iterate each out edge u->v
    if (hasPrunedOutNeighbors[u]==offset)
        for (EdgeID outEdgeLocation=outNeighborsLocator[u]; outEdgeLocation<prunedOutNeighborsEnd[u]; outEdgeLocation++) {
            VertexID& v = prunedOutNeighbors[outEdgeLocation].neighbor;
            if (useSearchOrderingStrategy && curPathEnd+1+backwardDist[v]>maxLen-4)
                break;

            // if next node not in stack
            if (inStack[v]==false) {
                EdgeID& edgeId = prunedOutNeighbors[outEdgeLocation].edgeId; 
                curPath[curPathEnd] = edgeId;
                curPathEnd++;
                inStack[v] = true;

                // if reach an arrival, try add edges in stack to results 
                if (isArrival[v]==offset) {
                    arrival = v;
                    if (tryAddEdges())
                        ans = true;
                }
                
                // continue forward search
                if (ans==false && curPathEnd+1<=maxLen-4 && forwardFinalSearch(v))
                    ans = true;

                // pop edge e(u,v) and vertex v from stacks
                curPathEnd--;
                inStack[v] = false;
                if (ans)
                    return true;
            }
        }

    return false;
}



// try add edges in current stack to results
bool Verification::tryAddEdges() {
    if (InDEnds[departure]+OutAEnds[arrival]>=2*maxLen-5) {
        addToResults();
        return true;
    }

    // obtain In_C
    short InCEnd = 0;
    for (short i=0; i<InDEnds[departure]; i++) {
        VertexID& a = InD[departure*(maxLen-2)+i];
        if (inStack[a]==false && InCEnd<2) {
            InC[InCEnd] = a;
            InCEnd++;
        }
    }
    if (InCEnd==0)
        return false;

    // obtain Out_C
    short OutCEnd = 0;
    for (short j=0; j<OutAEnds[arrival]; j++) {
        VertexID& b = OutA[arrival*(maxLen-2)+j];
        if (inStack[b]==false && OutCEnd<2) {
            OutC[OutCEnd] = b;
            OutCEnd++;
        }
    }
    if (OutCEnd==0)
        return false;

    // add to results
    if (InCEnd+OutCEnd>2 || InC[0]!=OutC[0]) {
        addToResults();
        return true;
    }
    return false;
}



// add edges in current path to final results
void Verification::addToResults() {
    for (VertexID k=0; k<curPathEnd; k++)
        if (isInResult[curPath[k]]<offset) {
            results[resultEnd] = curPath[k];
            resultEnd++;
            isInResult[curPath[k]] = offset;
        }
}



#endif