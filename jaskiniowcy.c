#define _XOPEN_SOURCE
#include <unistd.h>
#include <mpi.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#define GROUP_SIZE 1

#define REQUEST_FIELD 4
#define ACK_FIELD 5

#define REQUEST_CAVE 6
#define ACK_CAVE 7

#define NOT_QUEUED 123
#define QUEUED 654

#define true 1
#define false 0

struct{
	int state;
	int *group_size;
	char *group_queue;
	
} state;

int size,rank;

void cavemen(int no){
	
	alarm(1);
}

int main(int argc, char **argv){
	int j, m, n, k;
	if (argc != 5){
		printf("You have to type in all 4 numbers as parameters:\nJ - Cave capacity\nM - Minimum group size\nN - Maximum group size\nK - Number of Saint Moon Stones (K<J)\n\n");
		exit(EXIT_FAILURE);
	}
	else{
		j = atoi(argv[1]);
		m = atoi(argv[2]);
		n = atoi(argv[3]);
		k = atoi(argv[4]);
		if (m>=n || k>=j){
			printf("M = %d have to be smaller than N = %d and K = %d have to be smaller than J = %d!", m, n, k, j);
			exit(EXIT_FAILURE);
		}
	}

	//MPI Initialization
	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	state.group_size = calloc((size_t) size, sizeof(int));
	state.group_queue = calloc((size_t) size, sizeof(char));
	
	//Defining cavemen variables
	int i;
	for (i = 0; i<size; i++)
		state.group_queue[i] = NOT_QUEUED;

//1.
	//randomizing group size
	srand(time(0) + rank);
	state.group_size[rank] = m + ( rand() % ( n-m+1 ) );
//	printf("Grupa %d - %d jaskiniowców\n", rank, state.group_size[rank]);

	//sending this group size to others
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Send(&(state.group_size[rank]), 1, MPI_INT, i, GROUP_SIZE, MPI_COMM_WORLD);
	//receiving group sizes from others
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Recv(&(state.group_size[i]), 1, MPI_INT, i, GROUP_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
/*
	for (i=0; i<size; i++)
		printf("%d: grupa %d - %d jaskiniowców\n", rank, i, state.group_size[i]);
*/
	signal(SIGALRM, cavemen);
	alarm(1);	

	while(1){
		int recv;
		MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		printf("%d: received message\n", rank);
		switch (status.MPI_TAG){
			//Field entry request
			case REQUEST_FIELD:
				
			break;	
			//Field entry accept
			case ACK_FIELD:
			
			break;
			//Cave entry request
			case REQUEST_CAVE:
			
			break;
			//Cave entry accept
			case ACK_CAVE:
		
			break;

			
		}
	}

	MPI_Finalize();
}
