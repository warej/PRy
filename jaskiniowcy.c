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

//	States
#define	INITIAL       333	//	alg.: 1)
#define GO_FOR_STONE  334	//	alg.: 2)
#define GLADE_TO_CAVE 335	//	alg.: 3 - 6
#define WAIT_4_CEREM  336	//	alg.: 7 - 9
#define FINITO        337	//	alg.: 10 - 12


struct{
	int state = INITIAL;
	int *group_size;
	char *group_queue;
	
} state;

int size,rank;

//	Reaction for signal SIGALRM
void cavemen(int no){
	switch (state.state) {
		case INITIAL:
			break;
		case GO_FOR_STONE:
			break;
		case GLADE_TO_CAVE:
			break;
		case WAIT_4_CEREM:
			break;
		case FINITO:
			break;
	}

	alarm(1);
}


void init () {	//	alg.: 1)
	// Randomize group size
	//	TODO

	// Broadcast group size to others
	//	TODO

	// Increment status
}


void go_for_stone () {	//	alg.: 2)
	// Broadtcast request for entering Critical Section 1.

	// Increment status
}


void glade_to_cave () {	//	alg.: 3 - 6
	// If enough ACK_FIELD are recieved
	// TODO

		// If there is only 1 stone and I'm too big to enter the cave
		// then go back to GO_FOR_STONE
		// TODO

		// If there is no stone - wait for it
		// TODO

		// Grab stone and leave CS 1.
		// TODO

		// Random delay (?)

		// Broadtcast request for entering CS 2.

		// Increment status
}


void wait_4_cerem () {	//	alg.: 7 - 9
	// If enough ACK_CAVE are recieved
	//TODO

		// Enter the cave
		// TODO

		// Leave CS 2.
		// TODO

		// Increment status
}


void finalize_round () {	//	alg.: 10 - 12
	// If cave is full
	// TODO

		// Celebrate
		// TODO

		// Leave cave (broadcast)
		// TODO

		// Put back the stone
		// TODO

		// random delay (?)

		state.state = GO_FOR_STONE;

}


int main(int argc, char **argv){

	int j,	// Cave capacity
		m,	// Minimal group size
		n,	// Maximal group size
		k;	// Stones count
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
