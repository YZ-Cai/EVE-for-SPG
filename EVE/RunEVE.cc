/*
Test Essential Nodes Intersaction
Author: Yuzheng Cai
2022-01-12
------------------------------
C++ 11 
*/ 

#include "EVE.cc"
using namespace std;



int main(int argc, char *argv[]) {    

    // program input parameters
    if(argc < 4) {
        cout << "Usage: ./RunEVE <Graph File> <Query File> <Hop Constraint k>" << endl;
        exit(1);
    }
    graphFilename = extractFilename(argv[1]); 
    queryFilename = extractFilename(argv[2]); 
    maxLen = stoi(argv[3]);

    // basic logs
    logFile.open("../"+logPath, ios::app);
    outputBasicLogs("EVE");

    // initialize the graph
    Graph* graph = new Graph(("../"+datasetPath+graphFilename).c_str());

    // initialize the queries
    vector<PerQuery> queries;
    loadQueries(("../"+datasetPath+queryFilename).c_str(), queries);
    
    // EVE
    EVE* method = new EVE(graph);
    method->answerAllQueries(queries);
    method->cleanUp();

    return 0;
}

