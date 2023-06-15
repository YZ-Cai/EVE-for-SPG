# Source Codes of EVE Method for SPG Generation



This is the source codes of **EVE (<u>E</u>ssential <u>V</u>ertices based <u>E</u>xamination)** method for hop-constrained s-t simple path graph (SPG) generation. Link of the paper:

[Towards Generating Hop-constrained s-t Simple Path Graphs](https://doi.org/10.1145/3588915)

To run the codes, please read the following instructions.

<br/>


## 1 Prepare Datasets

Please copy your datasets into `Datasets`/. 

`Datasets/TestGraph1.graph` is an example for input graph format:

```
13
18
0,4
1,4
2,1
3,0
...
```

The first line is the number of vertices in graph (|V|), while the second line is the number of the edges (|E|).

In the following lines, each line represents a directed edge from u to v, separated by a comma ",". Note that vertex ids range from 0 to |V|-1, and edge ids range from 0 to |E|-1. For example, the third line is for edge 0->4, whose edge id is 0.

<br/>

## 2 Generate Queries

Usage of query generation program in `Datasets/GenQuery/`:

```shell
./GenerateQueries <Graph File> <Max length> <Number of queries>
```

- Graph File: input graph filename in  `Datasets/`
- Max length: the upper bound of query hop constraint k, i.e., k in [3, Max Length]
- Number of queries: the number of random queries generated for each k

For example, we can generate 10 random queries for each k in [3,8] with the following commands:

```shell
cd Datasets/GenQuery/
make
make clean
./GenerateQueries TestGraph1.graph 8 10
./GenerateQueries TestGraph2.graph 8 10
cd ../..
```

After executions, generated query files are stored as  `Datasets/{Graph Filename}_{k}.query`.

<br/>

## 3 Execute EVE Method

Usage of EVE main program in `EVE/`:

```
./RunEVE <Graph File> <Query File> <Hop Constraint k>
```

- Graph File: input graph filename in  `Datasets/`
- Query file: input query filename in  `Datasets/`
- Hop Constraint k: Hop constraint k for the input query file

```shell
cd EVE/
make
make clean
./RunEVE TestGraph1.graph TestGraph1.graph_6.query 6
./RunEVE TestGraph2.graph TestGraph2.graph_6.query 6
cd ../
```

After executions, the logs including running time are written in `Results/Logs.csv`.

The output edges (all edge ids in the desired simple path graph) for input queries are stored in `Results/Answers/{Query Filename}-{k}.EVE.answer`, in which each line is the answer of each query.

Statistics for answering each query are stored in `Results/Statistics/{Query Filename}-{k}.csv`, in which each line records the space cost, number of upper-bound edges and number of answer edges for each query.

Note that if you do not need to output answers (which may be very large for large graphs), please comment line 21 in file `Config.h`.  If you do not need to output statistics, please comment line 27 in file `Config.h`. For example:

```cpp
/*  
 * Input and Output  
*/

// whether to write answers to file
// #define WRITE_ANSWERS					// ! commented, and no answer file will be output

// answer file relative path
string answerPath = "Results/Answers/";

// whether to write statistics logs to file
// #define WRITE_STATISTICS					// ! commented, and no statistics file will be output

// statistics file relative path
string statisticsPath = "Results/Statistics/";
```

<br/>

## 4 Notes

In this version, we pre-allocate spaces for processing all queries efficiently. Thus, the memory consumed is larger than the actual space needed for a query. The actual space cost has been calculated in the output statistic file `Results/Statistics/{Query Filename}-{k}.csv`.

Please cite our paper [Towards Generating Hop-constrained s-t Simple Path Graphs](https://doi.org/10.1145/3588915) if you use these codes.
