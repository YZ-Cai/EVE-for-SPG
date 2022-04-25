/*
GraphUtils - basic utils
Author: Yuzheng Cai
2021-09-21
------------------------------
C++ 11 
*/ 


#ifndef UTILS_H
#define UTILS_H

#include "../Config.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include <algorithm>
#include <sys/time.h>
#include <math.h>
#include <random>
#include<string.h>

using namespace std;


// default graph
string graphFilename = "TestGraph1.graph";

// default query file
string queryFilename = "TestGraph1.graph_4.query";

// default hop constraint (>=3)
short maxLen=4;

// default answer type (exact/upperbound)
string answerType = "exact";



#if (defined _WIN32) || (defined _WIN64)
    #define WINDOWS 1
#else
    #define INT_MAX 2147483648
#endif


#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))
#define abs(a) ((a)>0?(a):(-(a)))
int getCeil(double value) {
    return int(ceil(value)+0.1);
}
int getFloor(double value) {
    return int(floor(value)+0.1);
}



// get current time in millisecond
inline double getCurrentTimeInMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + tv.tv_usec * 1e-3;
}



// get current time for writing logs
string getCurrentLogTime( )
{
    time_t now = time(0);
    tm *ltm = localtime(&now);
    return  to_string(1900 + ltm->tm_year) + "-" + to_string(1 + ltm->tm_mon) + "-" + to_string(ltm->tm_mday) + " " +
            to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min) + ":" + to_string(ltm->tm_sec);
}



// number to string
string str(float value, short fixed=2) {
    string result = to_string(value);
    if (abs(value-int(value))<1e-6)
        result = to_string(int(value));
    if (result.find(".")!=string::npos) 
        return result.substr(0, result.find(".") + fixed + 1);
    return result;
}



// extract filename
string extractFilename(string s) {
    short startPos=0, endPos=s.length();
    if (s.rfind("/")!=string::npos) 
        startPos = s.rfind("/")+1;
    return s.substr(startPos, endPos);
}



// output file stream
ofstream logFile, resultFile, statisticsFile;



// basic logs output
void outputBasicLogs(string methodName) {

    // to console
    printf("################ %s ################\n", methodName.c_str());
    printf("- Dataset: %s\n", graphFilename.c_str());
    printf("- Query File: %s\n", queryFilename.c_str());
    printf("- Start time: %s\n", getCurrentLogTime().c_str());
    printf("- Max length: %d\n", maxLen);

    // to log file
    logFile<<methodName<<","<<getCurrentLogTime()<<","<<graphFilename<<","<<queryFilename<<","<<maxLen<<",";
}



// get string of paramters
string getParaString() {
    return "-"+str(maxLen);
}



// storing each query
struct PerQuery {
    VertexID source, target;
};



// load query file
void loadQueries(const char* queryFilename, vector<PerQuery>& queries) {
    printf("Loading query file ...\n");
    VertexID fromId, toId;
    FILE* f = fopen(queryFilename, "r");
    while (fscanf(f, "%d,%d", &fromId, &toId) != EOF) 
        queries.push_back({fromId, toId});
    printf("- Finish. %d queries loaded\n", queries.size());
    logFile<<queries.size()<<",";
    fclose(f);
}



// common variables
int offset = 0;
VertexID *forwardFrontier, forwardFrontierEnd, *backwardFrontier, backwardFrontierEnd, *nextFrontier, nextFrontierEnd;
int *forwardDist, *backwardDist;



#endif