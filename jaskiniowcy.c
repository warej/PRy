/**
 * Lista rzeczy do zrobienia:
 * - sprawdzić, czy działa (a na razie nie powinno) dla wywołania pojedynczego procesu.
 *     Pewnie będzie potrzebne wysyłanie REQUESTa również do siebie samego.
 *
 */
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

	// Contains summary group size
	int sum_group_size;

	// Contains queue of groups waiting for response
	int *group_queue;

	// Table of processes having stones
	char *got_stone;

	// Tells if process is on the glade
	char on_the_glade;

	// Tells if process is in the cave
	char in_the_cave;
} state;

int size, // Count of all processes
	rank; // Number of current process

void cavemen (int no);


// Performs MPI brodcast
void send_to_all (void *data, int data_size, MPI_Datatype data_type, int msg_tag) {
	int i;
	for (i = 0; i < size; i++)
		//if (i != rank)
			MPI_Send(data, data_size, data_type, i, msg_tag, MPI_COMM_WORLD);
}


int count_free_stones () {
	int i, c = k;
	for (i = 0; i < size; i++)
		if (state.got_stone[i] == true)
			c--;

	return c;
}


// === Begining of state functions ===
// Init function
void init () {	//	alg.: 1)
	// Randomize group size
	srand(time(0) + rank);
	state.group_size[rank] = m + (rand() % (n-m+1) );
	printf("(%2d -> %2d): Grupa %d - %d jaskiniowców\n",
		rank, getpid(), rank, state.group_size[rank] );

	// Broadcast group size to others
	int i;
	for (i = 0; i < size; i++)
		if (i != rank)
			MPI_Send(&(state.group_size[rank]), 1, MPI_INT, i,
				GROUP_SIZE, MPI_COMM_WORLD);

	//receiving group sizes from others
	for (i = 0; i < size; i++)
		if (i != rank)
			MPI_Recv(&(state.group_size[i]), 1, MPI_INT, i,
				GROUP_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	/*	*/
	state.sum_group_size = 0;
	for (i = 0; i < size; i++) {
		state.sum_group_size = state.sum_group_size + state.group_size[i];
		//printf("(%2d): grupa %d - %d jaskiniowców\n", rank, i, state.group_size[i]);
	}
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
	int i, free_stones = count_free_stones();
	//printf("(%2d): There is still %d free stones\n", rank, free_stones);

	// If there is no stone - wait for it
	if (free_stones == 0) {
		signal(SIGALRM, cavemen);
		alarm(1);
		return;
	}

	// If there is only 1 stone and I'm too big to enter the cave
	// then go back to GO_FOR_STONE
	if (free_stones == 1) {
		int cave_free_space = j;
		for (i = 0; i < size; i++) {
			if (state.got_stone[i] == true) {
				cave_free_space = j - state.group_size[i];
			}
		}
		
		char someone_can = false;
		for (i = 0; i < size; i++)
			if (state.got_stone[i] == false && state.group_size[i] < cave_free_space)
				someone_can = true;

		if (state.group_size[rank] > cave_free_space && someone_can == true) {
			// Resign and let someone else try
			for (i = 0; i < size; i++) {
				if (state.group_queue[i] == WAITING_4_GLADE) {
					MPI_Send( (void*)&(state.group_size[rank]), 1, MPI_INT, i, ACK_FIELD, MPI_COMM_WORLD);
					state.group_queue[i] = NOT_QUEUED;
				}
			}
			state.on_the_glade = false;
			state.state = GO_FOR_STONE;
			signal(SIGALRM, cavemen);
			alarm(1);
			return;
		}
	}
	
	// Grab stone
	// B-cast PICK_STONE
	printf("(%2d): __Picking up the stone!__\n", rank);
	send_to_all( (void*)&(state.group_size[rank]),
		1,
		MPI_INT,
		PICK_STONE);

	// Leave Glade - reply to all waiting glade requests
	for (i = 0; i < size; i++) {
		if (state.group_queue[i] == WAITING_4_GLADE) {
			MPI_Send( (void*)&(state.group_size[rank]), 1, MPI_INT, i, ACK_FIELD, MPI_COMM_WORLD);
			state.group_queue[i] = NOT_QUEUED;
		}
	}
	state.on_the_glade = false;

	// Random delay (?)

	// Broadtcast request for entering CS 2.
	send_to_all( (void*)&(state.group_size[rank]), 1, MPI_INT, REQUEST_CAVE);
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
void cavemen (int no) {
	printf("(%2d): Got alarm!\n", rank);

	switch (state.state) {
		case GO_FOR_STONE:
			go_for_stone();
			// Alarm will be called in main loop,
			//   when enough ACKs are recievd
			break;
		case GLADE_TO_CAVE:
			glade_to_cave();
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
	state.in_the_cave = false;

	init();

	//printf("(%2d): Summary group size = %d\n", rank, state.sum_group_size);

	signal(SIGALRM, cavemen);
	alarm(1);	

	// Vars outside of loop
	int field_ack_counter = 0;
	int cave_ack_counter = 0, cave_ack_size = 0;

	while (true){
	/*	*/
	int recv;
	MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	switch (status.MPI_TAG){
		// Field entry request
		case REQUEST_FIELD:
			printf("(%2d): Recieved REQUEST_FIELD from (%2d)\n", rank, status.MPI_SOURCE);

			if (	status.MPI_SOURCE != rank && ( // It's me
						// - I'm on the glade
						state.on_the_glade == true
						// - or I'm waiting for the glade and my priority (rank)
						//   is higher then sender's
						|| (state.state == GO_FOR_STONE && rank > status.MPI_SOURCE) ) ) {

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
			if (field_ack_counter == size) {
				// Increment status
				state.on_the_glade = true;
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
			//printf("(%2d): %d grabbed a stone!\n", rank, status.MPI_SOURCE);
			break;

		// Some process left the stone
		case LEAVE_STONE:
			state.got_stone[status.MPI_SOURCE] = false;
			printf("(%2d): %d leaved a stone!\n", rank, status.MPI_SOURCE);

		// Cave entry request
		case REQUEST_CAVE:
			printf("(%2d): Recieved REQUEST_CAVE from (%2d)\n", rank, status.MPI_SOURCE);

			// Increment counters
			cave_ack_counter = cave_ack_counter + 1;
			cave_ack_size = cave_ack_size + state.group_size[status.MPI_SOURCE];
			if ( status.MPI_SOURCE == rank || state.got_stone[rank] == false || ( // It's me or I've got no stone or
						state.in_the_cave == false && ( // I've got stone but I'm not in the cave and
								state.state != GLADE_TO_CAVE // I'm not wating for the cave or
								|| rank < status.MPI_SOURCE// My priority is lower than requesting process
							)
						)
					) {

				// Reply with ACK_CAVE
				MPI_Send(&recv, 1, MPI_INT, status.MPI_SOURCE, ACK_CAVE, MPI_COMM_WORLD);
			} else {
				// Add requester to queue
				if (state.group_queue[status.MPI_SOURCE] != NOT_QUEUED)
					printf("(%2d): !ERR! Process queue[%2d] changed from %d to %d",
						rank, status.MPI_SOURCE,
						state.group_queue[status.MPI_SOURCE],  WAITING_4_CAVE);

				state.group_queue[status.MPI_SOURCE] = WAITING_4_CAVE;
			}
		break;

		// Cave entry accept
		case ACK_CAVE:
			printf("(%2d): Recieved ACK_CAVE from (%2d)\n", rank, status.MPI_SOURCE);
			
			// Increment counters
			cave_ack_counter = cave_ack_counter + 1;
			cave_ack_size = cave_ack_size + state.group_size[status.MPI_SOURCE];

			// If recieved enough ACKs to go into the cave
			if (state.in_the_cave == false && j >= (state.sum_group_size - cave_ack_size) ) {
				// Increment status
				state.in_the_cave = true;
				state.state = WAIT_4_CEREM;

				// Set alarm
				signal(SIGALRM, cavemen);
				alarm(1);
			} else if (state.in_the_cave == true && cave_ack_counter == size) {
				cave_ack_counter = 0;
				cave_ack_counter = cave_ack_counter - state.sum_group_size;
			}
		break;
		
		default:
			printf("(%2d): !!! Received unknown message from (%2d)\n", rank, status.MPI_SOURCE);
	}
	/*	*/	
	}

	MPI_Finalize();
}
