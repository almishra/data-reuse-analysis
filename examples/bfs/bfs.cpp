#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

#define OPEN

FILE *fp;

double total_in = 0;
double total_out = 0;

//Structure to hold a node information
struct Node
{
	int starting;
	int no_of_edges;
};

void BFSGraph(int argc, char** argv);

void Usage(int argc, char**argv){

fprintf(stderr,"Usage: %s <num_threads> <input_file>\n", argv[0]);

}
////////////////////////////////////////////////////////////////////////////////
// Main Program
////////////////////////////////////////////////////////////////////////////////
int main( int argc, char** argv) 
{
	BFSGraph( argc, argv);
}



////////////////////////////////////////////////////////////////////////////////
//Apply BFS on a Graph using CUDA
////////////////////////////////////////////////////////////////////////////////
void BFSGraph( int argc, char** argv) 
{
        int no_of_nodes = 0;
        int edge_list_size = 0;
        char *input_f;
	int	 num_omp_threads;
	
	if(argc!=3){
	Usage(argc, argv);
	exit(0);
	}

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

	num_omp_threads = atoi(argv[1]);
	input_f = argv[2];
	
	printf("Reading File\n");
	//Read in Graph from a file
	fp = fopen(input_f,"r");
	if(!fp)
	{
		printf("Error Reading graph file\n");
		return;
	}

	int source = 0;

	fscanf(fp,"%d",&no_of_nodes);
   
	// allocate host memory
	Node* h_graph_nodes = (Node*) malloc(sizeof(Node)*no_of_nodes);
	bool *h_graph_mask = (bool*) malloc(sizeof(bool)*no_of_nodes);
	bool *h_updating_graph_mask = (bool*) malloc(sizeof(bool)*no_of_nodes);
	bool *h_graph_visited = (bool*) malloc(sizeof(bool)*no_of_nodes);

	int start, edgeno;   
	// initalize the memory
	for( unsigned int i = 0; i < no_of_nodes; i++) 
	{
		fscanf(fp,"%d %d",&start,&edgeno);
		h_graph_nodes[i].starting = start;
		h_graph_nodes[i].no_of_edges = edgeno;
		h_graph_mask[i]=false;
		h_updating_graph_mask[i]=false;
		h_graph_visited[i]=false;
	}

	//read the source node from the file
	fscanf(fp,"%d",&source);
	// source=0; //tesing code line

	//set the source node as true in the mask
	h_graph_mask[source]=true;
	h_graph_visited[source]=true;

	fscanf(fp,"%d",&edge_list_size);

	int id,cost;
	int* h_graph_edges = (int*) malloc(sizeof(int)*edge_list_size);
	for(int i=0; i < edge_list_size ; i++)
	{
		fscanf(fp,"%d",&id);
		fscanf(fp,"%d",&cost);
		h_graph_edges[i] = id;
	}

	if(fp)
		fclose(fp);    


	// allocate mem for the result on host side
	int* h_cost = (int*) malloc( sizeof(int)*no_of_nodes);
	for(int i=0;i<no_of_nodes;i++)
		h_cost[i]=-1;
	h_cost[source]=0;
	
	printf("Start traversing the tree\n");
	
#ifdef OPEN
        double start_time = omp_get_wtime();
#ifdef OMP_OFFLOAD
#ifndef OMP_OFFLOAD_NOREUSE
        total_in += sizeof(int) + sizeof(bool)*no_of_nodes + sizeof(Node)*no_of_nodes + sizeof(int)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(int)*no_of_nodes;
        total_out += sizeof(int)*no_of_nodes;
#pragma omp target data map(to: no_of_nodes, h_graph_mask[0:no_of_nodes], h_graph_nodes[0:no_of_nodes], h_graph_edges[0:edge_list_size], h_graph_visited[0:no_of_nodes], h_updating_graph_mask[0:no_of_nodes]) map(h_cost[0:no_of_nodes])
#endif
        {
#endif 
#endif
	bool stop;
	do
        {
            //if no thread changes this value then the loop stops
            stop=false;

#ifdef OPEN
            omp_set_num_threads(num_omp_threads);
    #ifdef OMP_OFFLOAD
    #ifdef OMP_OFFLOAD_NOREUSE
        total_in += sizeof(int) + sizeof(bool)*no_of_nodes + sizeof(Node)*no_of_nodes + sizeof(int)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(int)*no_of_nodes;
        total_out += sizeof(int) + sizeof(bool)*no_of_nodes + sizeof(Node)*no_of_nodes + sizeof(int)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(int)*no_of_nodes;
    #pragma omp target data map(no_of_nodes, h_graph_nodes[0:no_of_nodes], h_graph_edges[0:edge_list_size], h_graph_visited[0:no_of_nodes], h_cost[0:no_of_nodes], h_updating_graph_mask[0:no_of_nodes], h_graph_mask[0:no_of_nodes])
    #endif
    #pragma omp target
    #endif
    #pragma omp parallel for 
#endif 
            for(int tid = 0; tid < no_of_nodes; tid++ )
            {
                if (h_graph_mask[tid] == true){ 
                    h_graph_mask[tid]=false;
                    for(int i=h_graph_nodes[tid].starting; i<(h_graph_nodes[tid].no_of_edges + h_graph_nodes[tid].starting); i++)
                    {
                        int id = h_graph_edges[i];
                        if(!h_graph_visited[id])
                        {
                            h_cost[id]=h_cost[tid]+1;
                            h_updating_graph_mask[id]=true;
                        }
                    }
                }
            }

#ifdef OPEN
    #ifdef OMP_OFFLOAD
    #ifdef OMP_OFFLOAD_NOREUSE
        total_in += sizeof(int) + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes;
        total_out += sizeof(int) + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes + sizeof(bool)*no_of_nodes;
    #pragma omp target data map(no_of_nodes, h_updating_graph_mask[0:no_of_nodes], h_graph_mask[0:no_of_nodes], h_graph_visited[0:no_of_nodes],stop)
    #endif
    #pragma omp target
    #endif
    #pragma omp parallel for
#endif
            for(int tid=0; tid< no_of_nodes ; tid++ )
            {
                if (h_updating_graph_mask[tid] == true){
                    h_graph_mask[tid]=true;
                    h_graph_visited[tid]=true;
                    stop=true;
                    h_updating_graph_mask[tid]=false;
                }
            }
        }
	while(stop);
#ifdef OPEN
        double end_time = omp_get_wtime();
        printf("Compute time: %lf\n", (end_time - start_time));
#ifdef OMP_OFFLOAD
        }
#endif
#endif
	//Store the result into a file
	FILE *fpo = fopen("result.txt","w");
	for(int i=0;i<no_of_nodes;i++)
		fprintf(fpo,"%d) cost:%d\n",i,h_cost[i]);
	fclose(fpo);
	printf("Result stored in result.txt\n");


    gettimeofday(&t2, NULL);
    double runtime = (t2.tv_sec - t1.tv_sec);
    runtime += (t2.tv_usec - t1.tv_usec) / 1000000.0;

    printf("Total Runtime - %.3f\n", runtime);

	// cleanup memory
	free( h_graph_nodes);
	free( h_graph_edges);
	free( h_graph_mask);
	free( h_updating_graph_mask);
	free( h_graph_visited);
	free( h_cost);

    printf("Total data transfered = %.3lf\n", (total_in + total_out) / 1024 / 1024);
}

