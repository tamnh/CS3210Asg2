/**
 * CS3210 - Assignment 2
 */

#include <mpi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

const int NUM_PLAYERS = 3;
const int NUM_PROCESSES = 4;
const int FIELD_PROCESS_RANK = 3;
const int FIELD_SIZES[2] = {128 , 64};
const int MAX_RUN = 10;
const int NUM_ROUNDS = 900;
const int BUFFER_SIZE = 9;


struct PlayerInfo {
	int prev_position[2];
	int current_position[2];
	int reached_ball_this_round;
	int win_ball_this_round;
	int num_meters_run;
	int num_times_reached_ball;
	int num_times_won_ball;
};


//math functions
int min(int x, int y) {
	if (x > y) {
		return y;
	} else {
		return x;
	}
}


int max(int x, int y) {
	if (x > y) {
		return x;
	} else {
		return y;
	}
}

int absolute(int x) {
	if (x < 0) {
		return -x;
	} else {
		return x;
	}
}

//helper functions
void print_player_info(struct PlayerInfo * player_info) {
	printf("%d %d %d %d %d %d %d %d %d\n", player_info->prev_position[0], player_info->prev_position[1], player_info->current_position[0], player_info->current_position[1], 
		player_info->reached_ball_this_round, player_info->win_ball_this_round, player_info->num_meters_run, player_info->num_times_reached_ball, player_info->num_times_won_ball);
}


bool is_field_process(int rank) {
	return rank == FIELD_PROCESS_RANK;
}


bool is_player_process(int rank) {
	return 0 <= rank && rank < NUM_PLAYERS;
}


void init_random_position(int * position) {
	position[0] = rand() % FIELD_SIZES[0];
	position[1] = rand() % FIELD_SIZES[1];
}


//receive either ball position or the initial postion;
void receive_info_from_field_process(int * buffer, int size, int tag) {
	MPI_Recv(buffer, size, MPI_INT, FIELD_PROCESS_RANK, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}


void receive_position_from_field_process(int * position, int tag) {
	receive_info_from_field_process(position, 2, tag);	
}


void init_player(struct PlayerInfo * player_info) {
	player_info->prev_position[0] = 0;
	player_info->prev_position[1] = 0;
	player_info->num_meters_run = 0;
	player_info->num_times_reached_ball = 0;
	player_info->num_times_won_ball = 0;
	player_info->win_ball_this_round = 0;
	player_info->reached_ball_this_round = 0;
}


void init_player_process(int rank, struct PlayerInfo * player_info, int * buffer) {
	int tag = 0;
	init_player(player_info);
	
	//receive the initial position
	receive_position_from_field_process(buffer, tag);
	player_info->current_position[0] = buffer[0];
	player_info->current_position[1] = buffer[1];

	//debug message
	//print_player_info(player_info);
}


void init_field_process(int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Request * requests) {
	int i;
	int tag = 0;
	init_random_position(ball_position);

	for (i=0; i<NUM_PLAYERS; i++) {
		init_random_position(buffers[i]);
		MPI_Isend(buffers[i], 2, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[i]);
	}

	MPI_Waitall(NUM_PLAYERS, requests, MPI_STATUSES_IGNORE);
}


void init_process(int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Request * requests) {
	if (is_field_process(rank)) {
		init_field_process(rank, player_info, buffers, ball_position, requests);
	} else {
		init_player_process(rank, &(player_info[0]), buffers[0]);
	}
}


void load_player_info_from_buffer(int * buffer, struct PlayerInfo * player_info) {
	player_info->prev_position[0] = buffer[0];
	player_info->prev_position[1] = buffer[1];
	player_info->current_position[0] = buffer[2];
	player_info->current_position[1] = buffer[3];
	player_info->reached_ball_this_round = buffer[4];
	player_info->num_meters_run = buffer[5];
	player_info->num_times_reached_ball = buffer[6];
	player_info->num_times_won_ball = buffer[7];
	player_info->win_ball_this_round = buffer[8];
}


void load_player_info_into_buffer(int * buffer, struct PlayerInfo * player_info) {
	buffer[0] = player_info->prev_position[0];
	buffer[1] = player_info->prev_position[1];
	buffer[2] = player_info->current_position[0];
	buffer[3] = player_info->current_position[1];
	buffer[4] = player_info->reached_ball_this_round;
	buffer[5] = player_info->num_meters_run;
	buffer[6] = player_info->num_times_reached_ball;
	buffer[7] = player_info->num_times_won_ball;
	buffer[8] = player_info->win_ball_this_round;
}


void send_info_to_field_process(int * buffer, int size, int tag) {
	MPI_Send(buffer, size, MPI_INT, FIELD_PROCESS_RANK, tag, MPI_COMM_WORLD);
}


void send_player_info_to_field_process(int * buffer, struct PlayerInfo * player_info, int tag) {
	load_player_info_into_buffer(buffer, player_info);
	send_info_to_field_process(buffer, BUFFER_SIZE, tag);
}


int get_distance(int * pos1, int * pos2) {
	return abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1]);
}
 

bool is_same_position(int * pos1, int * pos2) {
	return get_distance(pos1, pos2) == 0; 
}


void run_to_ball(int * src, int * dest) {
		//the directed distance
	int hor_dist = abs(dest[0] - src[0]);
	int ver_dist = abs(dest[1] - src[1]);

	if (hor_dist + ver_dist <= MAX_RUN) {
		src[0] = dest[0];
		src[1] = dest[1];
		return;
	}


	int diag_move = min(abs(hor_dist), abs(ver_dist));
	diag_move = min(diag_move, MAX_RUN / 2);


	// printf("%d %d %d %d %d \n", src[0], src[1], dest[0], dest[1], diag_move);
	int low = max(0, MAX_RUN - ver_dist);
	int high = min(hor_dist, MAX_RUN); 

	int hor_move = low + rand()% (high - low + 1);
	int ver_move = MAX_RUN - hor_move;

	if (src[0] > dest[0]) {
		src[0] = src[0] - hor_move;
	} else {
		src[0] = src[0] + hor_move;
	}


	if (src[1] > dest[1]) {
		src[1] = src[1] - ver_move;
	} else {
		src[1] = src[1] + ver_move;
	}

	//use left over moves to slide along 1 of the 2 directions
	/*
	int move_left = MAX_RUN - 2 * diag_move;

	if (src[0] > dest[0]) {
		src[0] = src[0] - move_left;
	} else if (src[0] < dest[0]) {
		src[0] = src[0] + move_left;
	}


	if (src[1] > dest[1]) {
		src[1] = src[1] - move_left;
	} else if (src[1] < dest[1]) {
		src[1] = src[1] + move_left;
	}
	*/
}


void run_player_round(int rank, struct PlayerInfo * player_info, int * buffer, int * ball_position) {
	int tag = 0;
	receive_position_from_field_process(ball_position, tag);

	player_info->prev_position[0] = player_info->current_position[0];
	player_info->prev_position[1] = player_info->current_position[1];
	
	run_to_ball(player_info->current_position, ball_position);

	
	// printf("%d %d %d \n", rank, ball_position[0], ball_position[1]);
	// print_player_info(player_info);

	// update the player info
	send_info_to_field_process(player_info->current_position, 2, tag);

	player_info->num_meters_run += get_distance(player_info->prev_position, player_info->current_position);
	player_info->reached_ball_this_round = 0;
	player_info->win_ball_this_round = 0;

	if (is_same_position(player_info->current_position, ball_position)) {
		player_info->reached_ball_this_round = 1;
		player_info->num_times_reached_ball ++;
		
		int is_winner = 0;
		receive_info_from_field_process(&is_winner, 1, tag);

		if (is_winner) {
			player_info->win_ball_this_round = 1;
			player_info->num_times_won_ball ++;
			int new_ball_position[2];
			init_random_position(new_ball_position);
			send_info_to_field_process(new_ball_position, 2, 0);
			// printf("New random position %d %d \n", new_ball_position[0], new_ball_position[1]);
		} 
	}

	send_player_info_to_field_process(buffer, player_info, tag);
}


void send_ball_position_to_players(int * ball_position, int tag, MPI_Request * requests) {
	int i;
	for (i = 0; i < NUM_PLAYERS; i++) {
		MPI_Isend(ball_position, 2, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[i]);
	}
	MPI_Waitall(NUM_PLAYERS, requests, MPI_STATUSES_IGNORE);
}


void receive_info_from_players(MPI_Request * requests, int ** buffers, struct PlayerInfo * players_info, int tag) {
	int i;
	
	for (i = 0; i < NUM_PLAYERS; i++) {
		MPI_Irecv(buffers[i], BUFFER_SIZE, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[i]);
	}

	MPI_Waitall(NUM_PLAYERS, requests, MPI_STATUSES_IGNORE);
	//load the information into 

	for (i = 0; i < NUM_PLAYERS; i++) {
		load_player_info_from_buffer(buffers[i], &players_info[i]);
	}
}


void run_field_round(int round, int rank, struct PlayerInfo * players_info, int ** buffers, int * ball_position, MPI_Request * requests) {
	int i;
	int tag = 0;
	
	printf("%d\n", round);
	printf("%d %d\n", ball_position[0], ball_position[1]);	
	
	send_ball_position_to_players(ball_position, tag, requests);
	//print rounds info


	//receive player positions
	for (i=0; i<NUM_PLAYERS; i++) {
		MPI_Irecv(buffers[i], 2, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[i]);
	}

	MPI_Waitall(NUM_PLAYERS, requests, MPI_STATUSES_IGNORE);

	int num_players_reached_ball = 0;

	for (i=0; i<NUM_PLAYERS; i++) {
		if (is_same_position(buffers[i], ball_position)) {
			num_players_reached_ball ++; 
		}
	}

	if (num_players_reached_ball > 0) {
		int winner = 0;
		int winner_id = rand() % num_players_reached_ball;
		int id = 0;

		for (i=0; i<NUM_PLAYERS; i++) {
			if (is_same_position(buffers[i], ball_position)) {
				if (id == winner_id) {
					buffers[i][0] = 1;
					MPI_Isend(buffers[i], 1, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[id]);
					winner = i;
				} else {
					buffers[i][0] = 0;
					MPI_Isend(buffers[i], 1, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[id]);
				}
				id++;
			}
		}

		MPI_Waitall(num_players_reached_ball, requests, MPI_STATUSES_IGNORE);

		MPI_Recv(ball_position, 2, MPI_INT, winner, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// printf("New random position %d %d \n", ball_position[0], ball_position[1]);	
	}	

	receive_info_from_players(requests, buffers, players_info, tag);

	for (i=0; i<NUM_PLAYERS; i++) {
		printf("%d ", i);
		print_player_info(&players_info[i]);
	}
}


void run_process_round(int round, int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Request * requests) {
	if (is_field_process(rank)) {
		run_field_round(round, rank, player_info, buffers, ball_position, requests);
	} else {
		run_player_round(rank, &player_info[0], buffers[0], ball_position);
	}
}


int main(int argc,char *argv[])
{
	int numtasks, rank, dest, source, rc, count, tag=1;
	int i;
	MPI_Status Stat;

	struct PlayerInfo * players_info;
	int ** buffers;
	int ball_position[2];
	MPI_Request * requests;

	srand(time(NULL));

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


	if (numtasks != NUM_PROCESSES) {
		printf("Expecting %d processors. Terminating.\n", NUM_PROCESSES);
		MPI_Finalize();
		return 0;
	}


	int num_buffers = 0;

	if (is_field_process(rank)) {
		num_buffers = NUM_PLAYERS;
	} else {
		num_buffers = 1;
	}

	players_info = (struct PlayerInfo *) malloc(num_buffers * sizeof (struct PlayerInfo));
	buffers = (int **) malloc(num_buffers * sizeof (int *));
	requests = (MPI_Request *) malloc(num_buffers * sizeof (MPI_Request));

	for (i=0; i<num_buffers; i++) {
		buffers[i] = (int *) malloc(BUFFER_SIZE * sizeof (int));
	}
	
	init_process(rank, players_info, buffers, ball_position, requests);

	int round = 1;
	while (round <= NUM_ROUNDS) {
		run_process_round(round, rank, players_info, buffers, ball_position, requests);
		MPI_Barrier(MPI_COMM_WORLD);
		round ++;
	}

	//free allocated memory
	/*
	if (is_field_process(rank)) {
		for (i=0; i<num_buffers; i++) {
			free(buffers[i]);
			free(players_info[i]);
		}
	} else {
		free(buffers[0]);
		free(players_info[0]);
	}
	*/

	MPI_Finalize();

	return 0;
}