/**
 * CS3210 - Assignment 2
 */

#include <mpi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

const int NUM_PLAYERS = 11;
const int NUM_PROCESSES = 12;
const int FIELD_PROCESS_RANK = 11;
const int FIELD_SIZES[2] = {128 , 64};
const int MAX_RUN = 10;
const int NUM_ROUNDS = 900;
const int BUFFER_SIZE = 7;

struct PlayerInfo {
	int initial_position[2];
	int final_position[2];
	int num_meters_run;
	int num_times_reached_ball;
	int num_times_won_ball;
};


//helper functions
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


void init_player(struct PlayerInfo * player_info) {
	player_info->initial_position[0] = 0;
	player_info->initial_position[1] = 0;
	init_random_position(player_info->final_position);
	player_info->num_meters_run = 0;
	player_info->num_times_reached_ball = 0;
	player_info->num_times_won_ball = 0;
}


void send_ball_position_to_players(MPI_Request * requests, int position) {

}


void receive_ball_position_from_field(int * position, int tag) {
	MPI_Recv(position, 2, MPI_INT, FIELD_PROCESS_RANK, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}


void load_player_info_from_buffer(int * buffer, struct PlayerInfo * player_info) {
	player_info->initial_position[0] = buffer[0];
	player_info->initial_position[1] = buffer[1];
	player_info->final_position[0] = buffer[2];
	player_info->final_position[1] = buffer[3];
	player_info->num_meters_run = buffer[4];
	player_info->num_times_reached_ball = buffer[5];
	player_info->num_times_won_ball = buffer[6];
}


void receive_info_from_players(MPI_Request* requests, int ** buffers, struct PlayerInfo ** players_info, int tag) {
	int i;
	
	for (i = 0; i < NUM_PLAYERS; i++) {
		MPI_Irecv(buffers[i], BUFFER_SIZE, MPI_INT, i, tag, MPI_COMM_WORLD, &requests[i]);
	}

	MPI_Waitall(NUM_PLAYERS, requests, MPI_STATUSES_IGNORE);
	//load the information into 

	for (i = 0; i < NUM_PLAYERS; i++) {

	}
}


int main(int argc,char *argv[])
{
	int numtasks, rank, dest, source, rc, count, tag=1;
	int i;
	MPI_Status Stat;

	struct PlayerInfo ** players_info;
	int ** buffers;
	int ball_position[2];

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


	if (numtasks != NUM_PROCESSES) {
		printf("Expecting %d processors. Terminating.\n", NUM_PROCESSES);
		MPI_Finalize();
		return 0;
	}

	//init 
	int num_buffers = 0;

	if (is_field_process(rank)) {
		num_buffers = NUM_PLAYERS;
	} else {
		num_buffers = 1;
	}


	players_info = (struct PlayerInfo **) malloc(NUM_PLAYERS * sizeof (struct PlayerInfo *));
	buffers = (int **) malloc(NUM_PLAYERS * sizeof (int *));
	
	for (i=0; i<num_buffers; i++) {
			buffers[i] = (int *) malloc(BUFFER_SIZE * sizeof (int));
			players_info[i] = (struct PlayerInfo *) malloc(sizeof (struct PlayerInfo));
	}


	if (is_field_process(rank)) {
		init_random_position(ball_position);
		printf("%d %d \n", ball_position[0], ball_position[1]);
	}

	int rounds_count = NUM_ROUNDS;

	while (rounds_count > 0) {
		if (is_field_process(rank)) {

		} else {

		}

		count --;
	}


	MPI_Finalize();
	

	//free allocated memory
	if (is_field_process(rank)) {
		for (i=0; i<NUM_PLAYERS; i++) {
			free(buffers[i]);
			free(players_info[i]);
		}
	} else {
		free(buffers[0]);
		free(players_info[0]);
	}


	free(buffers);
	free(players_info);
	free(ball_position);

	return 0;
}