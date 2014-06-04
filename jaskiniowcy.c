#define _XOPEN_SOURCE
#include <unistd.h>
#include <mpi.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define GROUP_SIZE 1

#define REQUEST_FIELD 4
#define APPROVAL_FIELD 5

#define REQUEST_CAVE 6
#define APPROVAL_CAVE 7

#define NO_MOON_STONE 239
#define HAVE_MOON_STONE 312

#define NOT_QUEUED 123
#define QUEUED 654

void gatherMoonStone(int &stone){
	stone = HAVE_MOON_STONE;
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
	int size,rank;
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	//Defining cavemen variables
	int group_size[size];
	int stone = NO_MOON_STONE;
	int group_queue[size];
	int i;
	for (i = 0; i<size; i++)
		group_queue[i] = NOT_QUEUED;

//1.
	//randomizing group size
	srand(time(0) + rank);
	group_size[rank] = m + ( rand() % ( n-m+1 ) );
/*	printf("Grupa %d - %d jaskiniowców\n", rank, group_size[rank]);
*/
	//sending this group size to others
	int i;
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Send(&(group_size[rank]), 1, MPI_INT, i, GROUP_SIZE, MPI_COMM_WORLD);
	//receiving group sizes from others
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Recv(&(group_size[i]), 1, MPI_INT, i, GROUP_SIZE, MPI_COMM_WORLD, MPI_IGNORE_STATUS);

/*	for (i=0; i<size; i++)
		printf("%d: grupa %d - %d jaskiniowców\n", rank, i, group_size[i]);
*/

//2.
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Send(rank, 1, MPI_INT, i, REQUEST_FIELD, MPI_COMM_WORLD);

	for (i=0; i<size-1; i++)
		MPI_Recv(NULL, 1, MPI_INT, MPI_ANY_SOURCE, APPROVAL_FIELD, MPI_COMM_WORLD, MPI_IGNORE_STATUS);

	gatherMoonStone(stone);
	if (stone == HAVE_MOON_STONE)
		printf("Cavemen group no. %d gathered moon_stone", rank);
	
	

	MPI_Finalize();
}
