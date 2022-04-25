/*
Essential Nodes Intersaction - implementations
Author: Yuzheng Cai
2022-02-27
------------------------------
C++ 11 
*/ 

#ifndef EVE_CC
#define EVE_CC
#include "EVE.h"
using namespace std;



EVE::EVE(Graph* inputGraph) {

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
    initEVE();
}


       
// experiments for answering all queries
void EVE::answerAllQueries(vector<PerQuery>& queries) {
    if (queries.size()==0) {
        printf("! No query\n");
        return;
    }
    
    // answers and statistics file
    #ifdef WRITE_ANSWERS
        resultFile.open("../"+answerPath+queryFilename+getParaString()+".EVE.answer");
        resultFile<<"number of edges,edge ids"<<endl;
    #endif
    #ifdef WRITE_STATISTICS
        queryId = 0;
        initStatisticStorage(queries.size());
    #endif

    // initialization
    printf("Running EVE ...\n");
    double startTime = getCurrentTimeInMs();

    // iterate each query
    for (PerQuery& query : queries) {

        // execute each query
        resultEnd = executeQuery(query.source, query.target, maxLen);
        
        // sort and write results to file
        #ifdef WRITE_ANSWERS
            sort(results, results+resultEnd);
            resultFile<<resultEnd;
            for (EdgeID j=0; j<resultEnd; j++)
                resultFile<<","<<results[j];
            resultFile<<endl;
        #endif
        #ifdef WRITE_STATISTICS
            queryId++;
        #endif
    }

    // output logs
    double timeCost = getCurrentTimeInMs() - startTime;
    printf("- Finish. Time cost: %.2f ms\n", timeCost);
    logFile<<str(timeCost)<<endl;

    // output answers and statistics file
    #ifdef WRITE_ANSWERS
        resultFile.close();
    #endif
    #ifdef WRITE_STATISTICS
        for (short i=0; i<queries.size(); i++) 
            statisticsFile<<spaceCosts[i]<<","<<numOfUpperbound[i]<<","<<numOfAnswers[i]<<endl;
        cleanUpStatisticStorage();
    #endif
}      



// execute for each query
EdgeID EVE::executeQuery(VertexID source, VertexID target, short k) {

    // initialization
    s = source;
    t = target;
    refreshMemory();

    // adaptive bi-directional BFS
    adaptiveBiDirectBFS();
    
    // forward and backward propagation to obtain essential vertices
    if (startPropDirection) {
        forwardPropagation();
        backwardPropagation();
    } else {
        backwardPropagation();
        forwardPropagation();
    }

    // iterate each candidate edge for edge labeling
    for (EdgeID i=0; i<candidateEnd; i++) {
        EdgeID& edgeId = candidates[i];
        VertexID& u = edges[edgeId].fromId;
        VertexID& v = edges[edgeId].toId;

        // edge labeling
        short label = edgeLabeling(edgeId);

        // definite edges
        if (label==2) {
            results[resultEnd] = edgeId;
            resultEnd++;
            if (maxLen>4 && u!=s && v!=t) {
                isInResult[edgeId] = offset;
                addToPrunedNeighbors(u, v, edgeId);
            }

        // undetermined edges
        } else if (label==1 && maxLen>4) {
            addToPrunedNeighbors(u, v, edgeId);
            edgesForVerification[edgesForVerificationEnd] = edgeId;
            edgesForVerificationEnd++;
        }

        // statistics
        #ifdef WRITE_STATISTICS
            if (label>0)
                numOfUpperbound[queryId]++;
        #endif
    }

    // verify each edge
    if (maxLen>4) 
        resultEnd = verification->verifyUndeterminedEdge(resultEnd, edgesForVerificationEnd, verticesHavingOutNeighborsEnd, verticesHavingInNeighborsEnd, departuresEnd, arrivalsEnd);
    
    // statistics
    #ifdef WRITE_STATISTICS
        numOfUpperbound[queryId] += specialCnt;
        numOfAnswers[queryId] = resultEnd; 
        spaceCosts[queryId] = getCurrentSpaceCost();
    #endif

    return resultEnd;
}



// initialization
void EVE::initEVE() {

    // propagation for essential vertices
    nextFrontier = new VertexID[VN];
    forwardFrontier = new VertexID[VN];
    backwardFrontier = new VertexID[VN];
    forwardDist = new int[VN]();
    backwardDist = new int[VN]();

    // propagation for essential vertices
    if (maxLen>2) {
        forwardEV0 = new VertexID[(maxLen-2)*VN*(maxLen-2)];
        backwardEV0 = new VertexID[(maxLen-2)*VN*(maxLen-2)];
        forwardEVLen0 = new int[VN*(maxLen-2)]();
        backwardEVLen0 = new int[VN*(maxLen-2)]();
        forwardLastLocation = new int[VN]();
        backwardLastLocation = new int[VN]();
        lastEV = new VertexID[VN*(maxLen-2)];
        lastEVEnds = new int[VN];
    }
    
    // edges in results
    results = new EdgeID[EN+1];
    candidates = results+1;
    isInResult = new int[EN]();

    // for verify each edge
    if (maxLen>4) {
        verification = new Verification(graph, results, isInResult);
        prunedOutNeighbors = verification->prunedOutNeighbors;
        prunedInNeighbors = verification->prunedInNeighbors;
        prunedOutNeighborsEnd = verification->prunedOutNeighborsEnd;
        prunedInNeighborsEnd = verification->prunedInNeighborsEnd;
        hasPrunedOutNeighbors = verification->hasPrunedOutNeighbors;
        hasPrunedInNeighbors = verification->hasPrunedInNeighbors;
        departures = verification->departures;
        arrivals = verification->arrivals;
        InD = verification->InD;
        OutA = verification->OutA;
        edgesForVerification = verification->edgesForVerification;

        // for search ordering strategy
        if (maxLen>6) {
            verticesHavingInNeighbors = verification->verticesHavingInNeighbors;
            verticesHavingOutNeighbors = verification->verticesHavingOutNeighbors;
        }   
    }
}                                                



// refresh memory for new query
inline void EVE::refreshMemory() {

    // refresh offset
    if (offset>=INT_MAX-maxLen-1) {
        offset = 0;
        if (maxLen>2) {
            memset(forwardEVLen0, 0, sizeof(int)*VN*(maxLen-2));
            memset(backwardEVLen0, 0, sizeof(int)*VN*(maxLen-2));
            memset(forwardLastLocation, 0, sizeof(int)*VN);
            memset(backwardLastLocation, 0, sizeof(int)*VN);
        }
        memset(isInResult, 0, sizeof(int)*EN);
        if (maxLen>4) 
            verification->refreshMemory();
    } 
    offset += maxLen+1;

    // refresh storages
    candidateEnd = resultEnd = edgesForVerificationEnd = 0;
    departuresEnd = 0;
    arrivalsEnd = 0;
    forwardMinId = s;
    forwardMaxId = s;
    backwardMinId = t;
    backwardMaxId = t;
    if (maxLen>6) {
        verticesHavingInNeighborsEnd = 0;
        verticesHavingOutNeighborsEnd = 0;
    }
    
    // statistics
    #ifdef WRITE_STATISTICS
        prunedNeighborsCount = 0;
        maxFrontierSize = 1;
        InDoutACount = 0;
        forwardEVCount = 0;
        backwardEVCount = 0;
        specialCnt = 0;
    #endif
}



// free up memories
void EVE::cleanUp() {

    // propagation for essential vertices
    delete[] nextFrontier;
    delete[] forwardFrontier;
    delete[] backwardFrontier;
    delete[] forwardDist;
    delete[] backwardDist;

    // propagation for essential vertices
    if (maxLen>2) {
        delete[] forwardEV0;
        delete[] backwardEV0;
        delete[] forwardEVLen0;
        delete[] backwardEVLen0;
        delete[] forwardLastLocation;
        delete[] backwardLastLocation;
        delete[] lastEV;
        delete[] lastEVEnds;
    }
    
    // results
    delete[] isInResult;
    delete[] results;

    // for verifying undetermined edges
    if (maxLen>4) 
        verification->cleanUp();
}



// add edges in upper-bound graph to new lists of neighbors
inline void EVE::addToPrunedNeighbors(VertexID& u, VertexID& v, EdgeID& edgeId) {
    if (hasPrunedInNeighbors[v]<offset) {
        hasPrunedInNeighbors[v] = offset;
        prunedInNeighbors[inNeighborsLocator[v]] = {edgeId, u};
        prunedInNeighborsEnd[v] = inNeighborsLocator[v]+1;
        if (maxLen>6) {
            verticesHavingInNeighbors[verticesHavingInNeighborsEnd] = v;
            verticesHavingInNeighborsEnd++;
        }
    } else {
        prunedInNeighbors[prunedInNeighborsEnd[v]] = {edgeId, u};
        prunedInNeighborsEnd[v]++;
    }
    if (hasPrunedOutNeighbors[u]<offset) {
        hasPrunedOutNeighbors[u] = offset;
        prunedOutNeighbors[outNeighborsLocator[u]] = {edgeId, v};
        prunedOutNeighborsEnd[u] = outNeighborsLocator[u]+1;
        if (maxLen>6) {
            verticesHavingOutNeighbors[verticesHavingOutNeighborsEnd] = u;
            verticesHavingOutNeighborsEnd++;
        }
    } else {
        prunedOutNeighbors[prunedOutNeighborsEnd[u]] = {edgeId, v};
        prunedOutNeighborsEnd[u]++;
    }

    // statistics
    #ifdef WRITE_STATISTICS
        prunedNeighborsCount += 2;
    #endif
}



// add edge id to candidates
inline void EVE::addToFinalCandidates(EdgeID& edgeId) {
    if (isInResult[edgeId]<offset-1) {
        candidates[candidateEnd] = edgeId;
        candidateEnd++;
        isInResult[edgeId] = offset-1;
    }
}        



// add a vertex u to essential vertices set
inline void EVE::addToEV(VertexID u, VertexID* EVStart, int EVLen) {
    int i = EVLen;
    while ( i>0 && EVStart[i-1]>u ) {
        EVStart[i] = EVStart[i-1];
        i--;
    }
    EVStart[i] = u;
}



// bi-directional BFS
void EVE::adaptiveBiDirectBFS() {
    
    // init
    backwardDist[s] = offset;
    backwardDist[t] = offset;
    forwardDist[s] = offset;
    forwardDist[t] = offset;
    forwardFrontier[0] = s;
    backwardFrontier[0] = t;
    forwardFrontierEnd = 1;
    backwardFrontierEnd = 1;
    short backwardMaxHop=0, forwardMaxHop=0;
    
    // explore one side until total length reaches maxLen
    while (backwardMaxHop+forwardMaxHop<maxLen) {
        nextFrontierEnd = 0;

        // expand forward frontier
        if (forwardFrontierEnd<backwardFrontierEnd) {
            forwardMaxHop++;

            // not the last step
            if (backwardMaxHop+forwardMaxHop<maxLen) 
                for (VertexID i=0; i<forwardFrontierEnd; i++) {
                    VertexID& u = forwardFrontier[i];

                    // iterate each edge u->v
                    for (EdgeID edgeLocator=outNeighborsLocator[u]; edgeLocator<outNeighborsLocator[u+1]; edgeLocator++) {
                        VertexID& v = outNeighbors[edgeLocator].neighbor;

                        // update d(s,v) and push to next frontier
                        if (forwardDist[v]<offset) {
                            forwardDist[v] = offset+forwardMaxHop;
                            forwardMinId = min(forwardMinId, v);
                            forwardMaxId = max(forwardMaxId, v);
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;
                        }
                    }
                }

            // is the last step
            else {
                continueDirection = true;
                for (VertexID i=0; i<forwardFrontierEnd; i++) {
                    VertexID& u = forwardFrontier[i];

                    // iterate each edge u->v
                    PerNeighbor* outNeighborsStart = outNeighbors+outNeighborsLocator[u];
                    PerNeighbor* outNeighborsEnd = outNeighbors+outNeighborsLocator[u+1];
                    if (outNeighborsEnd>outNeighborsStart && outNeighborsStart->neighbor<backwardMinId)
                        outNeighborsStart = lower_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMinId));
                    if (outNeighborsEnd>outNeighborsStart && (outNeighborsEnd-1)->neighbor>backwardMaxId)
                        outNeighborsEnd = upper_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMaxId));
                    for (; outNeighborsStart<outNeighborsEnd; outNeighborsStart++) {
                        VertexID& v = outNeighborsStart->neighbor;

                        // update d(s,v) and push to next frontier
                        if (forwardDist[v]<offset) {
                            if (backwardDist[v]>offset) {
                                forwardDist[v] = offset+forwardMaxHop;
                                forwardMinId = min(forwardMinId, v);
                                forwardMaxId = max(forwardMaxId, v);
                                nextFrontier[nextFrontierEnd] = v;
                                nextFrontierEnd++;
                            } else 
                                forwardDist[v] = offset;
                        }
                    }
                }
            }

            // swap frontier
            VertexID* tmp = forwardFrontier;
            forwardFrontier = nextFrontier;
            nextFrontier = tmp;
            forwardFrontierEnd = nextFrontierEnd;

            // statistics
            #ifdef WRITE_STATISTICS
                maxFrontierSize = max(forwardFrontierEnd, maxFrontierSize);
            #endif

        // expand backward frontier
        } else {
            backwardMaxHop++;

            // not the last step
            if (forwardMaxHop+backwardMaxHop<maxLen) 
                for (VertexID i=0; i<backwardFrontierEnd; i++) {
                    VertexID& u = backwardFrontier[i];
                    for (EdgeID edgeLocator=inNeighborsLocator[u]; edgeLocator<inNeighborsLocator[u+1]; edgeLocator++) {
                        VertexID& v = inNeighbors[edgeLocator].neighbor;

                        // update d(v,t) and push to next frontier
                        if (backwardDist[v]<offset) {
                            backwardDist[v] = offset+backwardMaxHop;
                            backwardMinId = min(backwardMinId, v);
                            backwardMaxId = max(backwardMaxId, v);
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;
                        }
                    }
                }

            // is the last step
            else {
                continueDirection = false;
                for (VertexID i=0; i<backwardFrontierEnd; i++) {
                    VertexID& u = backwardFrontier[i];

                    // iterate each in edge v->u
                    PerNeighbor* inNeighborsStart = inNeighbors+inNeighborsLocator[u];
                    PerNeighbor* inNeighborsEnd = inNeighbors+inNeighborsLocator[u+1];
                    if (inNeighborsEnd>inNeighborsStart && inNeighborsStart->neighbor<forwardMinId)
                        inNeighborsStart = lower_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMinId));
                    if (inNeighborsEnd>inNeighborsStart && (inNeighborsEnd-1)->neighbor>forwardMaxId)
                        inNeighborsEnd = upper_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMaxId));
                    for (; inNeighborsStart<inNeighborsEnd; inNeighborsStart++) {
                        VertexID& v = inNeighborsStart->neighbor;

                        // update d(v,t) and push to next frontier
                        if (backwardDist[v]<offset) {
                            if (forwardDist[v]>offset) {
                                backwardDist[v] = offset+backwardMaxHop;
                                backwardMinId = min(backwardMinId, v);
                                backwardMaxId = max(backwardMaxId, v);
                                nextFrontier[nextFrontierEnd] = v;
                                nextFrontierEnd++;
                            } else 
                                backwardDist[v] = offset;
                        }
                    }
                }
            }

            // swap frontier
            VertexID* tmp = backwardFrontier;
            backwardFrontier = nextFrontier;
            nextFrontier = tmp;
            backwardFrontierEnd = nextFrontierEnd;

            // statistics
            #ifdef WRITE_STATISTICS
                maxFrontierSize = max(backwardFrontierEnd, maxFrontierSize);
            #endif
        }
    }

    // continue forward search
    if (continueDirection) {
        for (short k=forwardMaxHop+1; k<=maxLen-1; k++) {
            nextFrontierEnd = 0;

            // each u in current frontier
            for (VertexID i=0; i<forwardFrontierEnd; i++) {
                VertexID& u = forwardFrontier[i];
                
                // iterate each out edge u->v
                PerNeighbor* outNeighborsStart = outNeighbors+outNeighborsLocator[u];
                PerNeighbor* outNeighborsEnd = outNeighbors+outNeighborsLocator[u+1];
                if (outNeighborsEnd>outNeighborsStart && outNeighborsStart->neighbor<backwardMinId)
                    outNeighborsStart = lower_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMinId));
                if (outNeighborsEnd>outNeighborsStart && (outNeighborsEnd-1)->neighbor>backwardMaxId)
                    outNeighborsEnd = upper_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMaxId));
                for (; outNeighborsStart<outNeighborsEnd; outNeighborsStart++) {
                    VertexID& v = outNeighborsStart->neighbor;
                    if (forwardDist[v]<offset) {

                        // update d(s,v) and push to next frontier
                        if ( backwardDist[v]>offset && k+backwardDist[v]-offset<=maxLen )  {
                            forwardDist[v] = offset+k;
                            forwardMinId = min(forwardMinId, v);
                            forwardMaxId = max(forwardMaxId, v);
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;
                        } else 
                            forwardDist[v] = offset;
                    }
                }
            }

            // swap frontier
            if (k<maxLen-1) {
                VertexID* tmp = forwardFrontier;
                forwardFrontier = nextFrontier;
                nextFrontier = tmp;
                forwardFrontierEnd = nextFrontierEnd;

                // statistics
                #ifdef WRITE_STATISTICS
                    maxFrontierSize = max(forwardFrontierEnd, maxFrontierSize);
                #endif
            }
        }
        startPropDirection = false;

    // continue backward search
    } else {
        for (short k=backwardMaxHop+1; k<=maxLen-1; k++) {
            nextFrontierEnd = 0;

            // each u in current frontier
            for (VertexID i=0; i<backwardFrontierEnd; i++) {
                VertexID& u = backwardFrontier[i];
                
                // each in edge v->u
                PerNeighbor* inNeighborsStart = inNeighbors+inNeighborsLocator[u];
                PerNeighbor* inNeighborsEnd = inNeighbors+inNeighborsLocator[u+1];
                if (inNeighborsEnd>inNeighborsStart && inNeighborsStart->neighbor<forwardMinId)
                    inNeighborsStart = lower_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMinId));
                if (inNeighborsEnd>inNeighborsStart && (inNeighborsEnd-1)->neighbor>forwardMaxId)
                    inNeighborsEnd = upper_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMaxId));
                for (; inNeighborsStart<inNeighborsEnd; inNeighborsStart++) {
                    VertexID& v = inNeighborsStart->neighbor;

                    // update d(v,t) and push to next frontier
                    if (backwardDist[v]<offset) {
                        if ( forwardDist[v]>offset && k+forwardDist[v]-offset<=maxLen )  {
                            backwardDist[v] = offset+k;
                            backwardMinId = min(backwardMinId, v);
                            backwardMaxId = max(backwardMaxId, v);
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;
                        } else 
                            backwardDist[v] = offset;
                    }
                }
            }

            // swap frontier
            if (k<maxLen-1) {
                VertexID* tmp = backwardFrontier;
                backwardFrontier = nextFrontier;
                nextFrontier = tmp;
                backwardFrontierEnd = nextFrontierEnd;

                // statistics
                #ifdef WRITE_STATISTICS
                    maxFrontierSize = max(backwardFrontierEnd, maxFrontierSize);
                #endif
            }
        }
        startPropDirection = true;
    }
}



// forward propagation
void EVE::forwardPropagation() {
    
    // preparation for next frontier
    forwardFrontierEnd = 0;

    // iterate each out edge s->v
    PerNeighbor* outNeighborsStart = outNeighbors+outNeighborsLocator[s];
    PerNeighbor* outNeighborsEnd = outNeighbors+outNeighborsLocator[s+1];
    if (outNeighborsEnd>outNeighborsStart && outNeighborsStart->neighbor<backwardMinId)
        outNeighborsStart = lower_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMinId));
    if (outNeighborsEnd>outNeighborsStart && (outNeighborsEnd-1)->neighbor>backwardMaxId)
        outNeighborsEnd = upper_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMaxId));
    for (; outNeighborsStart<outNeighborsEnd; outNeighborsStart++) {
        VertexID& v = outNeighborsStart->neighbor;
        EdgeID& edgeId = outNeighborsStart->edgeId;

        // satisfying d(v,t)+1<=maxLen
        if (backwardDist[v]>offset && backwardDist[v]-offset+1<=maxLen) {
            forwardFrontier[forwardFrontierEnd] = v;
            forwardFrontierEnd++;
            addToFinalCandidates(edgeId);

            // if start forward propagation first, d(s,v) should be recorded
            if (startPropDirection) {
                forwardDist[v] = offset+1;
                forwardMinId = min(forwardMinId, v);
                forwardMaxId = max(forwardMaxId, v);
            }

            // need to use essential vertices only when maxLen>3 
            if (maxLen>3) {
                forwardEV(1, v) = v;
                forwardEVLen(1, v) = offset+1;
                forwardLastLocation[v] = offset+1;
                lastEV[v*(maxLen-2)] = v;
                lastEVEnds[v] = 1;

                // statistics
                #ifdef WRITE_STATISTICS
                    forwardEVCount++;
                #endif
            }

        // special cases if edge e(s,t) exists when start forward propagation first
        } else if (startPropDirection && v==t) {
            results[resultEnd] = edgeId;
            resultEnd++;

            // statistics
            #ifdef WRITE_STATISTICS
                specialCnt++;
            #endif
        }
    }

    // from 2 hop to maxLen-2 hop
    for (int k=2; k<=maxLen-2; k++) {

        // initialize next frontier end
        nextFrontierEnd = 0;

        // each u in current frontier
        for (VertexID i=0; i<forwardFrontierEnd; i++) {
            VertexID& u = forwardFrontier[i];
            VertexID* uEVStart = &forwardEV(k-1, u);
            VertexID* uEVEnd = uEVStart + (forwardEVLen(k-1, u)-offset);         

            // iterate each out edge u->v
            PerNeighbor* outNeighborsStart = outNeighbors+outNeighborsLocator[u];
            PerNeighbor* outNeighborsEnd = outNeighbors+outNeighborsLocator[u+1];
            if (outNeighborsEnd>outNeighborsStart && outNeighborsStart->neighbor<backwardMinId)
                outNeighborsStart = lower_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMinId));
            if (outNeighborsEnd>outNeighborsStart && (outNeighborsEnd-1)->neighbor>backwardMaxId)
                outNeighborsEnd = upper_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMaxId));
            for (; outNeighborsStart<outNeighborsEnd; outNeighborsStart++) {
                VertexID& v = outNeighborsStart->neighbor;
                EdgeID& edgeId = outNeighborsStart->edgeId;

                // satisfying k + d(v,t) <= maxLen
                if (backwardDist[v]>offset && backwardDist[v]-offset+k<=maxLen) {

                    // if start forward propagation first, add to candidate edges
                    if (startPropDirection)
                        addToFinalCandidates(edgeId);

                    // essential set end of v in k step
                    VertexID* lastEVStart = lastEV + v*(maxLen-2);
                    int& lastEVLen = lastEVEnds[v];
                    int& vEVLenWithOffset = forwardEVLen(k, v);

                    // if v is not visited in k step
                    if (vEVLenWithOffset<offset) {
                        vEVLenWithOffset = offset;

                        // last essential vertices set is not exists
                        if (forwardLastLocation[v]<offset) {

                            // if start forward propagation first, d(s,v) should be recorded
                            if (startPropDirection) {
                                forwardDist[v] = offset+k;
                                forwardMinId = min(forwardMinId, v);
                                forwardMaxId = max(forwardMaxId, v);
                            }

                            // copy from u
                            lastEVLen = forwardEVLen(k-1, u)-offset;
                            memcpy(lastEVStart, uEVStart, lastEVLen*sizeof(VertexID));

                            // add to next frontier
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;

                        // do intersaction with last essential vertices set
                        } else 
                            if (lastEVLen>0) {
                                lastEVLen = set_intersection(lastEVStart, lastEVStart+lastEVLen, uEVStart, uEVEnd, lastEVStart) - lastEVStart;

                                // add to next frontier
                                nextFrontier[nextFrontierEnd] = v;
                                nextFrontierEnd++;
                            }
                    
                    // if v is visited in k step, do intersaction with last essential vertices set
                    } else if (lastEVLen>0)
                        lastEVLen = set_intersection(lastEVStart, lastEVStart+lastEVLen, uEVStart, uEVEnd, lastEVStart) - lastEVStart;
                }
            }
        }

        // statistics
        #ifdef WRITE_STATISTICS
            maxFrontierSize = max(nextFrontierEnd, maxFrontierSize);
        #endif

        // for each u in next frontier, add u to its essential vertices
        forwardFrontierEnd = 0;
        for (VertexID i=0; i<nextFrontierEnd; i++) {
            VertexID& u = nextFrontier[i];
            VertexID* uEVStart = &forwardEV(k, u);
            VertexID* lastEVStart = lastEV + u*(maxLen-2);
            int lastEVLen = lastEVEnds[u];
            if ( forwardLastLocation[u]<offset || lastEVLen+1 < forwardEVLen(forwardLastLocation[u]-offset, u)-offset ) {

                // copy from last essential vertices set to final essential vertices set
                memcpy(uEVStart, lastEVStart, lastEVLen*sizeof(VertexID));
                
                // add u to its essential vertices
                addToEV(u, uEVStart, lastEVLen);
                forwardEVLen(k, u) = offset + lastEVLen + 1;
                forwardLastLocation[u] = offset + k;
                
                // statistics
                #ifdef WRITE_STATISTICS
                    forwardEVCount += lastEVLen + 1;
                #endif

                // add to next frontier
                forwardFrontier[forwardFrontierEnd] = u;
                forwardFrontierEnd++;
            } else 
                forwardEVLen(k, u) = 0;
        }
    }

    // if start forward propagation first, continue for maxLen-1 hop
    if (startPropDirection && maxLen>2)
        for (VertexID i=0; i<forwardFrontierEnd; i++) {
            VertexID& u = forwardFrontier[i];       

            // iterate each out edge u->v
            PerNeighbor* outNeighborsStart = outNeighbors+outNeighborsLocator[u];
            PerNeighbor* outNeighborsEnd = outNeighbors+outNeighborsLocator[u+1];
            if (outNeighborsEnd>outNeighborsStart && outNeighborsStart->neighbor<backwardMinId)
                outNeighborsStart = lower_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMinId));
            if (outNeighborsEnd>outNeighborsStart && (outNeighborsEnd-1)->neighbor>backwardMaxId)
                outNeighborsEnd = upper_bound(outNeighborsStart, outNeighborsEnd, PerNeighbor(0, backwardMaxId));
            for (; outNeighborsStart<outNeighborsEnd; outNeighborsStart++) {
                VertexID& v = outNeighborsStart->neighbor;
                EdgeID& edgeId = outNeighborsStart->edgeId;

                // add to candidates, update d(s,v)
                if (backwardDist[v]==offset+1) {
                    addToFinalCandidates(edgeId);
                    if (forwardDist[v]<offset) {
                        forwardDist[v] = offset+maxLen-1;
                        forwardMinId = min(forwardMinId, v);
                        forwardMaxId = max(forwardMaxId, v);
                    }
                }
            }
        }
}



// backward propagation
void EVE::backwardPropagation() {
    
    // initialization
    backwardFrontierEnd = 0;

    // iterate each in edge v->s
    PerNeighbor* inNeighborsStart = inNeighbors+inNeighborsLocator[t];
    PerNeighbor* inNeighborsEnd = inNeighbors+inNeighborsLocator[t+1];
    if (inNeighborsEnd>inNeighborsStart && inNeighborsStart->neighbor<forwardMinId)
        inNeighborsStart = lower_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMinId));
    if (inNeighborsEnd>inNeighborsStart && (inNeighborsEnd-1)->neighbor>forwardMaxId)
        inNeighborsEnd = upper_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMaxId));
    for (; inNeighborsStart<inNeighborsEnd; inNeighborsStart++) {
        EdgeID& edgeId = inNeighborsStart->edgeId;
        VertexID& v = inNeighborsStart->neighbor;

        // satisfying d(s,v)+1<=maxLen
        if (forwardDist[v]>offset && forwardDist[v]-offset+1<=maxLen) {
            backwardFrontier[backwardFrontierEnd] = v;
            backwardFrontierEnd++;
            addToFinalCandidates(edgeId);

            // if start backward propagation first, d(v,t) should be recorded 
            if (!startPropDirection) {
                backwardDist[v] = offset+1;
                backwardMinId = min(backwardMinId, v);
                backwardMaxId = max(backwardMaxId, v);
            }
            
            // need to use essential vertices only when maxLen>3 
            if (maxLen>3) {
                backwardEV(1, v) = v;
                backwardEVLen(1, v) = offset+1;
                backwardLastLocation[v] = offset+1;
                lastEV[v*(maxLen-2)] = v;
                lastEVEnds[v] = 1;
                
                // statistics
                #ifdef WRITE_STATISTICS
                    backwardEVCount++;
                #endif
            }

        // special cases if edge e(s,t) exists when start backward propagation first
        } else if (!startPropDirection && v==s) {
            results[resultEnd] = edgeId;
            resultEnd++;
            
            // statistics
            #ifdef WRITE_STATISTICS
                specialCnt++;
            #endif
        }
    }

    // from 2 hop to maxLen-2 hop
    for (int k=2; k<=maxLen-2; k++) {

        // initialize next frontier end
        nextFrontierEnd = 0;

        // each u in current frontier
        for (VertexID i=0; i<backwardFrontierEnd; i++) {
            VertexID& u = backwardFrontier[i];
            VertexID* uEVStart = &backwardEV(k-1, u);
            VertexID* uEVEnd = uEVStart + (backwardEVLen(k-1, u)-offset);         

            // iterate each in edge v->u
            PerNeighbor* inNeighborsStart = inNeighbors+inNeighborsLocator[u];
            PerNeighbor* inNeighborsEnd = inNeighbors+inNeighborsLocator[u+1];
            if (inNeighborsEnd>inNeighborsStart && inNeighborsStart->neighbor<forwardMinId)
                inNeighborsStart = lower_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMinId));
            if (inNeighborsEnd>inNeighborsStart && (inNeighborsEnd-1)->neighbor>forwardMaxId)
                inNeighborsEnd = upper_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMaxId));
            for (; inNeighborsStart<inNeighborsEnd; inNeighborsStart++) {
                EdgeID& edgeId = inNeighborsStart->edgeId;
                VertexID& v = inNeighborsStart->neighbor;
                
                // satisfying k + d(s,v) <= maxLen
                if (forwardDist[v]>offset && forwardDist[v]-offset+k<=maxLen) {

                    // if start backward propagation first, add to candidate edges
                    if (!startPropDirection)
                        addToFinalCandidates(edgeId);

                    // essential set end of v in k step
                    VertexID* lastEVStart = lastEV + v*(maxLen-2);
                    int& lastEVLen = lastEVEnds[v];
                    int& vEVLenWithOffset = backwardEVLen(k, v);

                    // if v is not visited in k step
                    if (vEVLenWithOffset<offset) {
                        vEVLenWithOffset = offset;

                        // last essential vertices set is not exists 
                        if (backwardLastLocation[v]<offset) {

                            // if start backward propagation first, d(v,t) should be recorded
                            if (!startPropDirection) {
                                backwardDist[v] = offset+k;
                                backwardMinId = min(backwardMinId, v);
                                backwardMaxId = max(backwardMaxId, v);
                            }
                            
                            // copy from u
                            lastEVLen = backwardEVLen(k-1, u)-offset;
                            memcpy(lastEVStart, uEVStart, lastEVLen*sizeof(VertexID));

                            // add to next frontier
                            nextFrontier[nextFrontierEnd] = v;
                            nextFrontierEnd++;

                        // do intersaction with last essential vertices set
                        } else 
                            if (lastEVLen>0) {
                                lastEVLen = set_intersection(lastEVStart, lastEVStart+lastEVLen, uEVStart, uEVEnd, lastEVStart) - lastEVStart;

                                // add to next frontier
                                nextFrontier[nextFrontierEnd] = v;
                                nextFrontierEnd++;
                            }
                    
                    // if v is visited in k step, do intersaction with last essential vertices set
                    } else if (lastEVLen>0)
                        lastEVLen = set_intersection(lastEVStart, lastEVStart+lastEVLen, uEVStart, uEVEnd, lastEVStart) - lastEVStart; 
                }
            }
        }

        // statistics
        #ifdef WRITE_STATISTICS
            maxFrontierSize = max(nextFrontierEnd, maxFrontierSize);
        #endif

        // for each u in next frontier, add u to its essential vertices
        backwardFrontierEnd = 0;
        for (VertexID i=0; i<nextFrontierEnd; i++) {   
            VertexID& u = nextFrontier[i];
            VertexID* uEVStart = &backwardEV(k, u);
            VertexID* lastEVStart = lastEV + u*(maxLen-2);
            int lastEVLen = lastEVEnds[u];
            if ( backwardLastLocation[u]<offset || lastEVLen+1 < backwardEVLen(backwardLastLocation[u]-offset, u)-offset ) {

                // copy from last essential vertices set to final essential vertices set
                memcpy(uEVStart, lastEVStart, lastEVLen*sizeof(VertexID));
                
                // add u to its essential vertices
                addToEV(u, uEVStart, lastEVLen);
                backwardEVLen(k, u) = offset + lastEVLen + 1;
                backwardLastLocation[u] = offset + k;
                
                // statistics
                #ifdef WRITE_STATISTICS
                    backwardEVCount += lastEVLen + 1;
                #endif

                // add to next frontier
                backwardFrontier[backwardFrontierEnd] = u;
                backwardFrontierEnd++;
            } else 
                backwardEVLen(k, u) = 0;
        }
    }

    // if start backward propagation first, continue for maxLen-1 hop
    if (!startPropDirection && maxLen>2)
        for (VertexID i=0; i<backwardFrontierEnd; i++) {
            VertexID& u = backwardFrontier[i];       

            // iterate each in edge v->u
            PerNeighbor* inNeighborsStart = inNeighbors+inNeighborsLocator[u];
            PerNeighbor* inNeighborsEnd = inNeighbors+inNeighborsLocator[u+1];
            if (inNeighborsEnd>inNeighborsStart && inNeighborsStart->neighbor<forwardMinId)
                inNeighborsStart = lower_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMinId));
            if (inNeighborsEnd>inNeighborsStart && (inNeighborsEnd-1)->neighbor>forwardMaxId)
                inNeighborsEnd = upper_bound(inNeighborsStart, inNeighborsEnd, PerNeighbor(0, forwardMaxId));
            for (; inNeighborsStart<inNeighborsEnd; inNeighborsStart++) {
                VertexID& v = inNeighborsStart->neighbor;
                EdgeID& edgeId = inNeighborsStart->edgeId;

                // add to candidates, update d(v,t)
                if (forwardDist[v]==offset+1) {
                    addToFinalCandidates(edgeId);
                    if (backwardDist[v]<offset) {
                        backwardDist[v] = offset+maxLen-1;
                        backwardMinId = min(backwardMinId, v);
                        backwardMaxId = max(backwardMaxId, v);
                    }
                } 
            }
        }
}



// intersact for each candidate edge
short EVE::edgeLabeling(EdgeID& edgeId) {
    VertexID& u = edges[edgeId].fromId;
    VertexID& v = edges[edgeId].toId;

    // edge linking from s
    if (u==s) {
        if (backwardDist[v]>offset)
            return 2;
        return 0;
    }

    // edge linking to t
    if (v==t) {
        if (forwardDist[u]>offset)
            return 2;
        return 0;
    }

    // s->u->v -> ... -> t
    bool flag = false;
    bool flag1 = false;
    if (forwardDist[u]==offset+1) {

        // k2 = 1
        if (backwardDist[v]==offset+1) {
            if (maxLen>4) {
                if (isDeparture[v]==offset) {
                    short& InDEnd = InDEnds[v];
                    if (InDEnd<maxLen-2) {
                        InD[v*(maxLen-2)+InDEnd] = u;
                        InDEnd++;
                    }
                } else {
                    departures[departuresEnd] = v;
                    departuresEnd++;
                    isDeparture[v] = offset;
                    InD[v*(maxLen-2)] = u;
                    InDEnds[v] = 1;
                }
                if (isArrival[u]==offset) {
                    short& OutAEnd = OutAEnds[u];
                    if (OutAEnd<maxLen-2) {
                        OutA[u*(maxLen-2)+OutAEnd] = v;
                        OutAEnd++;
                    }
                } else {
                    arrivals[arrivalsEnd] = u;
                    arrivalsEnd++;
                    isArrival[u] = offset;
                    OutA[u*(maxLen-2)] = v;
                    OutAEnds[u] = 1;
                }
                
                // statistics
                #ifdef WRITE_STATISTICS
                    InDoutACount += 2;
                #endif
            }
            flag = true;
            flag1 = true;
        }

        // k2 = 2 to maxLen-2
        if (flag==false)
            for (int k2=2; k2<=min(maxLen-2, backwardLastLocation[v]-offset); k2++) 
                if (backwardEVLen(k2, v)>=offset) {
                    VertexID* vEVStart = &backwardEV(k2, v);
                    VertexID* vEVEnd = vEVStart + (backwardEVLen(k2, v)-offset);

                    // check have intersaction or not
                    bool isEmpty = true;
                    while (vEVStart != vEVEnd)
                        if (*vEVStart==u) {
                            isEmpty = false;
                            break;
                        } else
                            vEVStart++;

                    // if no intersaction
                    if (isEmpty) {
                        if (maxLen>4) {
                            if (isDeparture[v]==offset) {
                                short& InDEnd = InDEnds[v];
                                if (InDEnd<maxLen-2) {
                                    InD[v*(maxLen-2)+InDEnd] = u;
                                    InDEnd++;
                                }
                            } else {
                                departures[departuresEnd] = v;
                                departuresEnd++;
                                isDeparture[v] = offset;
                                InD[v*(maxLen-2)] = u;
                                InDEnds[v] = 1;
                            }
                            
                            // statistics
                            #ifdef WRITE_STATISTICS
                                InDoutACount += 1;
                            #endif
                        }
                        flag = true;
                        break;
                    }
                }
    }
        

    // s-> ... ->u->v->t
    if (flag1==false)
        if (backwardDist[v]==offset+1) 
            for (int k1=2; k1<=min(maxLen-2, forwardLastLocation[u]-offset); k1++) 
                if (forwardEVLen(k1, u)>=offset) {
                    VertexID* uEVStart = &forwardEV(k1, u);
                    VertexID* uEVEnd = uEVStart + (forwardEVLen(k1, u)-offset);

                    // check have intersaction or not
                    bool isEmpty = true;
                    while (uEVStart != uEVEnd)
                        if (*uEVStart==v) {
                            isEmpty = false;
                            break;
                        } else
                            uEVStart++;

                    // if no intersaction
                    if (isEmpty) {
                        if (maxLen>4) {
                            if (isArrival[u]==offset) {
                                short& OutAEnd = OutAEnds[u];
                                if (OutAEnd<maxLen-2) {
                                    OutA[u*(maxLen-2)+OutAEnd] = v;
                                    OutAEnd++;
                                }
                            } else {
                                arrivals[arrivalsEnd] = u;
                                arrivalsEnd++;
                                isArrival[u] = offset;
                                OutA[u*(maxLen-2)] = v;
                                OutAEnds[u] = 1;
                            }
                            
                            // statistics
                            #ifdef WRITE_STATISTICS
                                InDoutACount += 1;
                            #endif
                        }
                        return 2;
                    }
                }    
    if (flag)
        return 2;

    // s-> ... -> u->v -> ... ->t
    for (int k1=2; k1<=min(maxLen-3, forwardLastLocation[u]-offset); k1++) 
        if (forwardEVLen(k1, u)>=offset) {
            VertexID* uEVStart = &forwardEV(k1, u);
            VertexID* uEVEnd = uEVStart + (forwardEVLen(k1, u)-offset);

            // v->t
            int k2 = min(maxLen-k1-1, backwardLastLocation[v]-offset);
            while (k2>=2 && backwardEVLen(k2, v)<offset)
                k2--;
            if (k2>=2) {
                VertexID* uEVStart1 = uEVStart;
                VertexID* uEVEnd1 = uEVEnd;
                VertexID* vEVStart = &backwardEV(k2, v);
                VertexID* vEVEnd = vEVStart + (backwardEVLen(k2, v)-offset);

                // check have intersaction or not
                bool isEmpty = true;
                while (uEVStart1 != uEVEnd1 && vEVStart != vEVEnd)
                    if ((*uEVStart1)<(*vEVStart))
                        ++uEVStart1;
                    else if ((*uEVStart1)>(*vEVStart))
                        ++vEVStart;
                    else {
                        isEmpty = false;
                        break;
                    }
                if (isEmpty)
                    return 1;
            }
        }

    return 0;
}         



// for statistics
#ifdef WRITE_STATISTICS

    // return space cost of current query
    double EVE::getCurrentSpaceCost() {
        double spaceCost = 0;

        // propagation for essential vertices
        spaceCost += sizeof(VertexID)*maxFrontierSize;                      // nextFrontier = new VertexID[VN];
        spaceCost += sizeof(VertexID)*maxFrontierSize;                      // forwardFrontier = new VertexID[VN];
        spaceCost += sizeof(VertexID)*maxFrontierSize;                      // backwardFrontier = new VertexID[VN];
        spaceCost += sizeof(int)*VN;                                        // forwardDist = new int[VN]();
        spaceCost += sizeof(int)*VN;                                        // backwardDist = new int[VN]();

        // propagation for essential vertices
        if (maxLen>2) {
            spaceCost += sizeof(VertexID)*forwardEVCount;                   // forwardEV0 = new VertexID[(maxLen-2)*VN*(maxLen-2)];
            spaceCost += sizeof(VertexID)*backwardEVCount;                  // backwardEV0 = new VertexID[(maxLen-2)*VN*(maxLen-2)];
            spaceCost += sizeof(int)*VN*(maxLen-2);                         // forwardEVLen0 = new int[VN*(maxLen-2)]();
            spaceCost += sizeof(int)*VN*(maxLen-2);                         // backwardEVLen0 = new int[VN*(maxLen-2)]();
            spaceCost += sizeof(int)*VN;                                    // forwardLastLocation = new int[VN]();
            spaceCost += sizeof(int)*VN;                                    // backwardLastLocation = new int[VN]();
            spaceCost += sizeof(VertexID)*(maxLen-2);                       // lastEV = new VertexID[VN*(maxLen-2)];
            spaceCost += sizeof(int)*VN;                                    // lastEVEnds = new int[VN];
        }

        // edges in results
        spaceCost += sizeof(EdgeID)*(candidateEnd+1);                       // results = new EdgeID[EN+1]; candidates = results+1;
        spaceCost += sizeof(int)*EN;                                        // isInResult = new int[EN]();

        // for verify each edge
        if (maxLen>4) {
            spaceCost += verification->getCurrentSpaceCost();               // get space cost of verification
            spaceCost += sizeof(PerNeighbor)*prunedNeighborsCount;          // prunedOutNeighbors and prunedInNeighbors are not calculated in verification
            spaceCost += sizeof(VertexID)*InDoutACount;                     // InD and OutA are not calculated in verification
        }

        return spaceCost;
    }       

    void EVE::initStatisticStorage(int queryNumber){
        numOfAnswers = new EdgeID[queryNumber]();
        numOfUpperbound = new EdgeID[queryNumber]();
        spaceCosts = new double[queryNumber];

        // open statistics file
        statisticsFile.open("../"+statisticsPath+extractFilename(queryFilename)+getParaString()+".csv");
        statisticsFile<<"Space cost (bytes),# Upper-bound edges,# Answer edges"<<endl;
        
    }

    void EVE::cleanUpStatisticStorage(){
        delete[] numOfAnswers;
        delete[] numOfUpperbound;
        delete[] spaceCosts;

        // close statistics file
        statisticsFile.close();
    }

#endif



#endif