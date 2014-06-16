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
#define PICK_STONE 10
#define LEAVE_STONE 11

#define REQUEST_CAVE 6
#define ACK_CAVE 7

#define NOT_QUEUED 123
#define WAITING_4_GLADE 654
#define WAITING_4_CAVE 282

#define true 1
#define false 0

//	States
// #define	INITIAL       333	//	alg.: 1)
#define GO_FOR_STONE  334	//	alg.: 2)
#define GLADE_TO_CAVE 335	//	alg.: 3 - 6
#define WAIT_4_CEREM  336	//	alg.: 7 - 9
#define FINITO        337	//	alg.: 10 - 12

int j,	// Cave capacity
	m,	// Minimal group size
	n,	// Maximal group size
	k;	// Stones count

struct{
	// Contains current process state
	int state;

	// Contains sizes of each group
	int *group_size;

	// Contains queue of groups waiting for response
	int *group_queue;

	// Table of processes having stones
	char *got_stone;

	// Tells if process is on the glade
	char on_the_glade;
} state;

int size, // Count of all processes
	rank; // Number of current process


// Performs MPI brodcast
void send_to_all (void *data, int data_size, MPI_Datatype data_type, int msg_tag) {
	int i;
	for (i = 0; i < size; i++)
		if (i != rank)
			MPI_Send(data, data_size, data_type, i, msg_tag, MPI_COMM_WORLD);
}


// === Begining of state functions ===
// Init function
void init () {	//	alg.: 1)
	// Randomize group size
	srand(time(0) + rank);
	state.group_size[rank] = m + ( rand() % (n-m+1) );
	printf("(%2d -> %2d): Grupa %d - %d jaskiniowców\n",
		rank, getpid(), rank, state.group_size[rank] );

	// Broadcast group size to others
	int i;
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Send(&(state.group_size[rank]), 1, MPI_INT, i,
				GROUP_SIZE, MPI_COMM_WORLD);

	//receiving group sizes from others
	for (i=0; i<size; i++)
		if (i != rank)
			MPI_Recv(&(state.group_size[i]), 1, MPI_INT, i,
				GROUP_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	/*	*/
	for (i=0; i<size; i++)
		printf("(%2d): grupa %d - %d jaskiniowców\n", rank, i, state.group_size[i]);
	/*	*/

	// Increment status
	state.state = GO_FOR_STONE;
}


// Going for stone
void go_for_stone () {	//	alg.: 2)
	// Broadcast request for entering Critical Section 1.
	send_to_all( (void*)&(state.group_size[rank]),
		1,
		MPI_INT,
		REQUEST_FIELD);
}


// Entering the glade, picking up the stone, going to cave
//   and try to enter the cave
void glade_to_cave () {	//	alg.: 3 - 6
	printf("(%2d): glade_to_cave not implemented yet!\n", rank);
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
	printf("(%2d): wait_4_cerem not implemented yet!\n", rank);
	// If enough ACK_CAVE are recieved
	//TODO

	// Enter the cave
	// TODO

	// Leave CS 2.
	// TODO

	// Increment status
}


void finalize_round () {	//	alg.: 10 - 12
	printf("(%2d): finalize_round not implemented yet!\n", rank);
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


//	Reaction for signal SIGALRM
void cavemen (int no){
	printf("(%2d): Got alarm!\n", rank);

	switch (state.state) {
		case GO_FOR_STONE:
			go_for_stone();
			// Alarm will be called in main loop,
			//   when enough ACKs are recievd
			break;
		case GLADE_TO_CAVE:
			glade_to_cave();
			signal(SIGALRM, cavemen);
			alarm(1);
			break;
		case WAIT_4_CEREM:
			wait_4_cerem();
			signal(SIGALRM, cavemen);
			alarm(1);
			break;
		case FINITO:
			finalize_round();
			signal(SIGALRM, cavemen);
			alarm(1);
			break;
		default:
			perror("Undefined state!");
	}
}


int main (int argc, char **argv) {
	if (argc != 5) {
		printf("You have to type in all 4 numbers as parameters:\nJ - Cave capacity\nM - Minimum group size\nN - Maximum group size\nK - Number of Saint Moon Stones (K<J)\n\n");
		exit(EXIT_FAILURE);
	}
	else {
		j = atoi(argv[1]);
		m = atoi(argv[2]);
		n = atoi(argv[3]);
		k = atoi(argv[4]);
		if (m>=n || k>=j) {
			printf("M = %d have to be smaller than N = %d and K = %d have to be smaller than J = %d!", m, n, k, j);
			exit(EXIT_FAILURE);
		}
	}

	//MPI Initialization
	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	//Defining cavemen variables
	state.group_size = calloc( (size_t) size, sizeof(int) );
	state.group_queue = calloc( (size_t) size, sizeof(int) );
	state.got_stone = calloc( (size_t) size, sizeof(char) );
	int i;
	for (i = 0; i<size; i++) {
		state.group_queue[i] = NOT_QUEUED;
		state.got_stone[i] = false;
	}
	state.on_the_glade = false;

	init();

	signal(SIGALRM, cavemen);
	alarm(1);	

	// Vars outside of loop
	int field_ack_counter = 0;

	while (true){
	/*	*/
	int recv;
	MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	switch (status.MPI_TAG){
		// Field entry request
		case REQUEST_FIELD:
			printf("(%2d): Recieved REQUEST_FIELD from (%2d)\n", rank, status.MPI_SOURCE);

			if (	// - I'm on the glade
					state.on_the_glade == true
					// - or I'm waiting for the glade and my priority (rank)
					//   is higher then sender's
					|| (state.state == GO_FOR_STONE && rank > status.MPI_SOURCE) ) {

				// Add source process to queue
				if (state.group_queue[status.MPI_SOURCE] != NOT_QUEUED)
					printf("(%2d): !ERR! Process queue[%2d] changed from %d to %d",
						rank, status.MPI_SOURCE,
						state.group_queue[status.MPI_SOURCE],  WAITING_4_GLADE);

				state.group_queue[status.MPI_SOURCE] = WAITING_4_GLADE;
			}
			else {
				// Reply with ACK_FIELD
				MPI_Send(&recv, 1, MPI_INT, status.MPI_SOURCE, ACK_FIELD, MPI_COMM_WORLD);
			}
		break;	
		
		// Field entry accept
		case ACK_FIELD:
			printf("(%2d): Recieved ACK_FIELD from (%2d)\n", rank, status.MPI_SOURCE);
			field_ack_counter = field_ack_counter + 1;
			if (field_ack_counter == size - 1) {
				// Increment status
				state.state = GLADE_TO_CAVE;
				field_ack_counter = 0;

				// Set alarm
				signal(SIGALRM, cavemen);
				alarm(1);
			}
		break;

		// Process on field took 1 stone
		case PICK_STONE:
			state.got_stone[status.MPI_SOURCE] = true;
			break;

		// Some process left the stone
		case LEAVE_STONE:
			state.got_stone[status.MPI_SOURCE] = false;

		// Cave entry request
		case REQUEST_CAVE:
			printf("(%2d): Recieved REQUEST_CAVE from (%2d)\n", rank, status.MPI_SOURCE);
			// TODO
		break;

		// Cave entry accept
		case ACK_CAVE:
			printf("(%2d): Recieved ACK_CAVE from (%2d)\n", rank, status.MPI_SOURCE);
			// TODO
		break;
		
		default:
			printf("(%2d): !!! Received unknown message from (%2d)\n", rank, status.MPI_SOURCE);
	}
	/*	*/	
	}

	MPI_Finalize();
}
