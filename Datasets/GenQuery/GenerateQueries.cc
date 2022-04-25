/*
Generate Reachable Queries for different k
Author: Yuzheng Cai
2022-03-18
------------------------------
C++ 11 
*/ 

#include "../../GraphUtils/Graph.cc"
#include<cstdlib>
using namespace std;


int main(int argc, char *argv[]) {    

    // program input parameters
    if(argc < 4) {
        cout << "Usage: ./GenerateQueries <Graph File> <max length> <number of queries>" << endl;
        exit(1);
    }
    graphFilename = extractFilename(argv[1]); 
    int minLen = 3; 
    int maxLen = stoi(argv[2]);
    int numOfQueries = stoi(argv[3]);

    // initialize the graph
    Graph* graph = new Graph(("../"+graphFilename).c_str());
    VertexID VN = graph->VN;
    PerNeighbor* outNeighbors = graph->outNeighbors;
    EdgeID* outNeighborsLocator = graph->outNeighborsLocator;

    // initialization for query generation
    VertexID* frontier = new VertexID[VN];
    VertexID frontierEnd;
    nextFrontier = new VertexID[VN];
    VertexID* visitedVertices = new VertexID[VN];
    VertexID visitedVerticesEnd = 0;
    int* isVisited = new int[VN]();
    vector<vector<PerQuery>> queries;
    for (int i=0; i<=maxLen; i++)
        queries.push_back({});
    int currentCount = 0;
	srand(2022);

    // generate queries
    cout<<"Generating queries ..."<<endl;
    while (currentCount<numOfQueries) {
        if (offset>=INT_MAX-1) {
            memset(isVisited, 0, sizeof(int)*VN);
            offset = 0;
        }
        offset++;

        // generate source vertex for new queries        
        VertexID s = rand()%VN;

        // BFS from s to find k-hop reachable target vertices
        frontier[0] = s;
        frontierEnd = 1;
        isVisited[0] = offset;
        visitedVerticesEnd = 0;
        for (int k=1; k<=maxLen; k++) {
            nextFrontierEnd = 0;

            // expand neighbors in frontier
            for (int i=0; i<frontierEnd; i++) {
                VertexID& u = frontier[i];
                for (EdgeID locator=outNeighborsLocator[u]; locator<outNeighborsLocator[u+1]; locator++) {
                    VertexID& v = outNeighbors[locator].neighbor;

                    if (isVisited[v]<offset) {
                        isVisited[v] = offset;
                        visitedVertices[visitedVerticesEnd] = v;
                        visitedVerticesEnd++;
                        nextFrontier[nextFrontierEnd] = v;
                        nextFrontierEnd++;
                    }
                }
            }

            // swap frontier
            VertexID* tmp = frontier;
            frontier = nextFrontier;
            nextFrontier = tmp;
            frontierEnd = nextFrontierEnd;

            // generate target vertex for a new query
            if (k>=minLen && visitedVerticesEnd>0) {
                VertexID t = visitedVertices[rand()%visitedVerticesEnd];
                queries[k].push_back({s, t});
            }
        }

        if (visitedVerticesEnd>0) {
            currentCount++;
            if (currentCount%100==0)
                cout<<"Generated "<<currentCount<<" queries."<<endl;
        }
    }

    // write all queries
    ofstream queryFile;
    for (int k=minLen; k<=maxLen; k++) {
        queryFile.open("../"+graphFilename+"_"+to_string(k)+".query");
        for (auto& query : queries[k])
            queryFile<<(query.source)<<","<<(query.target)<<endl;
        queryFile.close();
    }

    return 0;    
}

