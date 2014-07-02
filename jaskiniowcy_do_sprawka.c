int j,	// Cave capacity
	m,	// Minimal group size
	n,	// Maximal group size
	k;	// Stones count

struct{
	int state;	// Contains current process state
	int *group_size;	// Contains sizes of each group
	int sum_group_size;	// Contains summary group size
	int *group_queue;	// Contains queue of groups waiting for response
	char *got_stone;	// Table of processes having stones
	char on_the_glade;	// Tells if process is on the glade
	char *in_the_cave;	// Tells if process is in the cave
} state;

int size, // Count of all processes
	rank; // Number of current process

void send_to_all (void *data, int data_size, MPI_Datatype data_type, int msg_tag) {
	int i;
	for (i = 0; i < size; i++) MPI_Send(data, data_size, data_type, i, msg_tag, MPI_COMM_WORLD);
}

int count_free_stones () {
	int i, c = k;
	for (i = 0; i < size; i++)
		if (state.got_stone[i] == true) c--;
	return c;
}

void init () {
	// Randomize group size
	srand(time(0) + rank);
	state.group_size[rank] = m + (rand() % (n-m+1) );
	// Broadcast group size to others
	int i;
	for (i = 0; i < size; i++)
		if (i != rank)
			MPI_Send(&(state.group_size[rank]), 1, MPI_INT, i, GROUP_SIZE, MPI_COMM_WORLD);
	//receiving group sizes from others
	for (i = 0; i < size; i++)
		if (i != rank)
			MPI_Recv(&(state.group_size[i]), 1, MPI_INT, i, GROUP_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	state.sum_group_size = 0;
	for (i = 0; i < size; i++)
		state.sum_group_size = state.sum_group_size + state.group_size[i];
	// Increment status
	state.state = GO_FOR_STONE;
}

void go_for_stone () {
	// Broadcast request for entering Critical Section 1.
	send_to_all( &null, 1, MPI_INT, REQUEST_FIELD);
}

void glade_to_cave () {
	int i, free_stones = count_free_stones();
	if (free_stones == 0) {
		signal(SIGALRM, cavemen);
		alarm(1);
		return;
	}
	send_to_all( (void*)&(state.group_size[rank]), 1, MPI_INT, PICK_STONE);
	for (i = 0; i < size; i++)
		if (state.group_queue[i] == WAITING_4_GLADE) {
			MPI_Send(&null, 1, MPI_INT, i, ACK_FIELD, MPI_COMM_WORLD);
			state.group_queue[i] = NOT_QUEUED;
		}
	state.on_the_glade = false;
	send_to_all( &null, 1, MPI_INT, REQUEST_CAVE);
}

char can_any_group_get_in(int free_stones){
	int sum_in_cave = 0, i, min = n;
	for (i = 0; i<size; i++){
		if (state.in_the_cave[i] == true)
			sum_in_cave += state.group_size[i];
		else if (state.group_size[i] < min && (free_stones > 0 || state.got_stone[i]))
			min = state.group_size[i];
	}
	if (min <= j-sum_in_cave || state.sum_group_size == sum_in_cave) return true;
	else return false;
}

void wait_4_cerem () {
	int free_stones = count_free_stones();
	if(can_any_group_get_in(free_stones) == false)
		send_to_all( &null, 1, MPI_INT, CELEBRATE);
}

void ceremony () {
	send_to_all(&null, 1, MPI_INT, LEAVE_CAVE);
	send_to_all(&null, 1, MPI_INT, LEAVE_STONE);
	int i;
	for (i = 0; i < size; i++)
		if (state.group_queue[i] == WAITING_4_CAVE) {
			if (state.in_the_cave[i] == false)
				MPI_Send(&null, 1, MPI_INT, i, ACK_CAVE, MPI_COMM_WORLD);
			state.group_queue[i] = NOT_QUEUED;
		}
	state.state = GO_FOR_STONE;
}

//	Reaction for signal SIGALRM
void cavemen (int no) {
	switch (state.state) {
		case GO_FOR_STONE:
			go_for_stone();
			break;
		case GLADE_TO_CAVE:
			glade_to_cave();
			break;
		case WAIT_4_CEREM:
			wait_4_cerem();
			signal(SIGALRM, cavemen);
			alarm(1);
			break;
		case CEREMONY:
			ceremony();
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
	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	state.group_size = calloc( (size_t) size, sizeof(int) );
	state.group_queue = calloc( (size_t) size, sizeof(int) );
	state.got_stone = calloc( (size_t) size, sizeof(char) );
	state.in_the_cave = calloc( (size_t) size, sizeof(char) );
	int i;
	for (i = 0; i<size; i++) {
		state.group_queue[i] = NOT_QUEUED;
		state.got_stone[i] = false;
		state.in_the_cave[i] = false;
	}
	state.on_the_glade = false;
	init();
	signal(SIGALRM, cavemen);
	alarm(1);	

	// Vars outside of loop
	int field_ack_counter = 0;
	int cave_ack_counter = 0, cave_ack_size = 0;
	while (true){
		int recv;
		MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		switch (status.MPI_TAG){
			case REQUEST_FIELD:
				if (status.MPI_SOURCE != rank && (
							state.on_the_glade == true
							|| (state.state == GO_FOR_STONE && rank > status.MPI_SOURCE) ) ) {
					state.group_queue[status.MPI_SOURCE] = WAITING_4_GLADE;
				} else {
					MPI_Send(&null, 1, MPI_INT, status.MPI_SOURCE, ACK_FIELD, MPI_COMM_WORLD);
				}
				break;	
			case ACK_FIELD:
				field_ack_counter = field_ack_counter + 1;
				if (field_ack_counter == size) {
					// Increment status
					state.on_the_glade = true;
					state.state = GLADE_TO_CAVE;
					field_ack_counter = 0;
					signal(SIGALRM, cavemen);
					raise(SIGALRM);
				}
				break;
			case PICK_STONE:
				state.got_stone[status.MPI_SOURCE] = true;
				break;
			case LEAVE_STONE:
				state.got_stone[status.MPI_SOURCE] = false;
				break;
			case REQUEST_CAVE:
				if ( status.MPI_SOURCE == rank || state.got_stone[rank] == false || (
							state.in_the_cave[rank] == false && (
									state.state != GLADE_TO_CAVE
									|| rank < status.MPI_SOURCE))) {
					MPI_Send(&null, 1, MPI_INT, status.MPI_SOURCE, ACK_CAVE, MPI_COMM_WORLD);
				} else {
					state.group_queue[status.MPI_SOURCE] = WAITING_4_CAVE;
				}
				break;
			case ACK_CAVE:
				if (state.state == GLADE_TO_CAVE){
					cave_ack_counter = cave_ack_counter + 1;
					cave_ack_size = cave_ack_size + state.group_size[status.MPI_SOURCE];
					// If received enough ACKs to go into the cave
					if (j - state.group_size[rank] >= (state.sum_group_size - cave_ack_size) ) {
						state.in_the_cave[rank] = true;
						state.state = WAIT_4_CEREM;
						send_to_all(&null, 1, MPI_INT, ENTER_CAVE);
						cave_ack_counter = 0;
						cave_ack_size = 0;
						signal(SIGALRM, cavemen);
						alarm(1);
					}
				}
				break;
			case ENTER_CAVE:
				state.in_the_cave[status.MPI_SOURCE] = true;
				state.group_queue[status.MPI_SOURCE] = NOT_QUEUED;
				break;
			case CELEBRATE:
				if (state.state == WAIT_4_CEREM)
					state.state = CEREMONY;
				break;
			case LEAVE_CAVE:
				state.in_the_cave[status.MPI_SOURCE] = false;
				break;
			default:
				printf("(%2d): !!! Received unknown message from (%2d)\n", rank, status.MPI_SOURCE);
		}
	}
	MPI_Finalize();
}
