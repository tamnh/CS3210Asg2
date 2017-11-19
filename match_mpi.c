/**
 * CS3210 - Assignment 2
 */

#include <mpi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>


const int FIELD_PROCESS_RANK = 0;
const int FIELD_SIZES[2] = {128 , 96};
const int SUBFIELD_SIZES[2] = {32, 32};
const int SUBFIELD_COUNTS[2] = {4, 3};

const int MAX_RUN = 10;
const int NUM_FIELD_PROCESS = 12;
const int TEAM_SIZES = 11;
const int NUM_ROUNDS = 2700;
const int BUFFER_SIZE = 10;
const int NUM_PROCESSES = NUM_FIELD_PROCESS + 2 * TEAM_SIZES;

const int TEAM_A_FIRST_ID = NUM_FIELD_PROCESS;
const int TEAM_A_LAST_ID = TEAM_A_FIRST_ID + TEAM_SIZES - 1;
const int TEAM_B_FIRST_ID = NUM_FIELD_PROCESS + TEAM_SIZES;
const int TEAM_B_LAST_ID = TEAM_B_FIRST_ID + TEAM_SIZES - 1;

//unique key to create communication channels
const int REPORT_COMM_KEY = 100;

const int ATTRIBUTES_LIST[22][3] = {
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5},
	{5, 5, 5}
};


struct PlayerInfo {
	int prev_position[2];
	int current_position[2];
	int reached_ball_this_round;
	int win_ball_this_round;
	int num_meters_run;
	int num_times_reached_ball;
	int num_times_won_ball;
	int dribbling_skill;
	int speed_skill;
	int kick_power;
};

bool is_first_half = true;

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
	printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", player_info->prev_position[0], player_info->prev_position[1], player_info->current_position[0], player_info->current_position[1], 
		player_info->reached_ball_this_round, player_info->win_ball_this_round, player_info->num_meters_run, player_info->num_times_reached_ball, player_info->num_times_won_ball,
		player_info->speed_skill, player_info->dribbling_skill, player_info->kick_power);
}


bool is_team_A_player(int rank) {
	return TEAM_A_FIRST_ID <= rank && rank <= TEAM_A_LAST_ID;
}


bool is_team_B_player(int rank) {
	return TEAM_B_FIRST_ID <= rank && rank <= TEAM_B_LAST_ID;
}


bool is_field_process(int rank) {	
	return 0 <= rank && rank < NUM_FIELD_PROCESS;
}


void init_random_position(int * position) {
	position[0] = rand() % (FIELD_SIZES[0] / 2) ;
	position[1] = rand() % FIELD_SIZES[1];

	if (!is_first_half) {
		position[0] = FIELD_SIZES[0] - 1 - position[0];
	}
}


//receive either ball position or the initial postion;
void receive_info_from_field_process(int * buffer, int size, int tag) {
	MPI_Recv(buffer, size, MPI_INT, FIELD_PROCESS_RANK, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}


void receive_position_from_field_process(int * position, int tag) {
	receive_info_from_field_process(position, 2, tag);	
}


void init_player(struct PlayerInfo * player_info, int rank) {
	//position variables
	player_info->prev_position[0] = 0;
	player_info->prev_position[1] = 0;
	init_random_position(player_info->current_position);
	
	player_info->num_meters_run = 0;
	player_info->num_times_reached_ball = 0;
	player_info->num_times_won_ball = 0;
	player_info->win_ball_this_round = 0;
	player_info->reached_ball_this_round = 0;

	int attr_idx = rank - NUM_FIELD_PROCESS;
	player_info->speed_skill = ATTRIBUTES_LIST[attr_idx][0];
	player_info->dribbling_skill = ATTRIBUTES_LIST[attr_idx][1];
	player_info->kick_power = ATTRIBUTES_LIST[attr_idx][2];
}


void reset_player_position(struct PlayerInfo * player_info) {
	init_random_position(player_info->current_position);
}


void init_player_process(int rank, struct PlayerInfo * player_info, MPI_Comm * report_comm) {
	int tag = 0;
	init_player(player_info, rank);
	MPI_Comm_split(MPI_COMM_WORLD, REPORT_COMM_KEY, rank, report_comm);
	// print_player_info(player_info);
}


void init_field_process(int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Comm * subfield_comm, MPI_Comm * report_comm) {
	int i;
	int tag = 0;

	MPI_Comm_split(MPI_COMM_WORLD, rank, rank, subfield_comm);

	if (rank == 0) {
		init_random_position(ball_position);
		MPI_Comm_split(MPI_COMM_WORLD, REPORT_COMM_KEY, rank, report_comm);
	} else {
		MPI_Comm_split(MPI_COMM_WORLD, rank, rank, report_comm);
	}
}


void init_process(int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Comm * all_subfields_comm, MPI_Comm * subfield_comm, MPI_Comm * report_comm) {
	if (is_field_process(rank)) {
		init_field_process(rank, player_info, buffers, ball_position, subfield_comm, report_comm);
	} else {
		init_player_process(rank, &player_info[0], report_comm);
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


int get_distance(int * pos1, int * pos2) {
	return abs(pos1[0] - pos2[0]) + abs(pos1[1] - pos2[1]);
}
 

bool is_same_position(int * pos1, int * pos2) {
	return get_distance(pos1, pos2) == 0; 
}


int get_field_index(int * position) {
	int idx_0 = position[0] / FIELD_SIZES[0];
	int idx_1 = position[1] / FIELD_SIZES[1];

	int field_index = idx_0 + idx_1 * SUBFIELD_COUNTS[0];
	return field_index;
}


//move player toward the ball
void run_to_ball(int * src, int * dest, int speed_skill) {
	//the directed distance
	int max_run = min(MAX_RUN, speed_skill);

	int hor_dist = abs(dest[0] - src[0]);
	int ver_dist = abs(dest[1] - src[1]);

	if (hor_dist + ver_dist <= max_run) {
		src[0] = dest[0];
		src[1] = dest[1];
		return;
	}


	// printf("%d %d %d %d %d \n", src[0], src[1], dest[0], dest[1], diag_move);
	int low = max(0, max_run - ver_dist);
	int high = min(hor_dist, max_run); 

	int hor_move = low + rand()% (high - low + 1);
	int ver_move = max_run - hor_move;

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


void send_player_info_to_field_process(int * buffer, struct PlayerInfo * player_info, MPI_Comm * report_comm) {
	load_player_info_into_buffer(buffer, player_info);
	MPI_Gather(buffer, BUFFER_SIZE, MPI_INT, NULL, BUFFER_SIZE, MPI_INT, 0, *report_comm);
	MPI_Barrier(* report_comm);
}


void challenge_ball(int rank, struct PlayerInfo * player_info, int * buffer, MPI_Comm * subfield_comm, int * ball_position) {
	int field_id = get_field_index(player_info->current_position);

	MPI_Comm_split(MPI_COMM_WORLD, field_id, rank, subfield_comm);

	MPI_Barrier(* subfield_comm);

	MPI_Comm_free(subfield_comm);
}


void run_player_round(int rank, struct PlayerInfo * player_info, int * buffer, int * ball_position, MPI_Comm * subfield_comm, MPI_Comm * report_comm) {
	//sync ball position 
	MPI_Bcast(ball_position, 2, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	
	//move all player to the ball
	player_info->prev_position[0] = player_info->current_position[0];
	player_info->prev_position[1] = player_info->current_position[1];
	run_to_ball(player_info->current_position, ball_position, player_info->speed_skill);


	// printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
	send_player_info_to_field_process(buffer, player_info, report_comm);
}


void handle_field_challenges(int rank, int ** buffers, MPI_Comm * subfield_comm) {
	

	MPI_Barrier(* subfield_comm);

	MPI_Comm_free(subfield_comm);
}


void receive_players_info(int ** buffers, struct PlayerInfo * players_info, MPI_Comm * report_comm) {
	int i;
	int * dummy_buffer = (int *) malloc (BUFFER_SIZE * sizeof (int));
	
	MPI_Gather(dummy_buffer, BUFFER_SIZE, MPI_INT, buffers[0], BUFFER_SIZE, MPI_INT, 0, *report_comm);
	MPI_Barrier(* report_comm);

	for (i=1; i<=2*TEAM_SIZES; i++) {
		load_player_info_from_buffer(buffers[i], &players_info[i]);
	}
}


void run_field_round(int round, int rank, struct PlayerInfo * players_info, int ** buffers, int * ball_position, MPI_Request * requests, MPI_Comm * subfield_comm, MPI_Comm * report_comm) {
	int i;

	if (rank == 0) {
		//we don't store the position of the ball
		printf("%d\n", round);
		printf("%d %d\n", ball_position[0], ball_position[1]);
	}

	MPI_Bcast(ball_position, 2, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	// printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
	if (get_field_index(ball_position) == rank) {
		handle_field_challenges(rank, buffers, subfield_comm);
	}

	if (rank == 0) {
		receive_players_info(buffers, players_info, report_comm);
		for (i=1; i<=2*TEAM_SIZES; i++) {
			print_player_info(&players_info[i]);
		}
	}
}


void run_process_round(int round, int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Request * requests, MPI_Comm * subfield_comm, MPI_Comm * report_comm) {
	if (is_field_process(rank)) {
		//printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
		run_field_round(round, rank, player_info, buffers, ball_position, requests, subfield_comm, report_comm);
	} else {
		// printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
		run_player_round(rank, &player_info[0], buffers[0], ball_position, subfield_comm, report_comm);
	}
}


int main(int argc,char *argv[])
{
	int numtasks, rank, dest, source, rc, count, tag=1;
	int i;
	MPI_Status Stat;

	struct PlayerInfo * players_info;

	int ** buffers;
	int * flat_buffer;

	int ball_position[2];
	MPI_Request * requests;

	MPI_Comm all_subfields_comm;
	MPI_Comm subfield_comm;
	MPI_Comm report_comm;

	//set different seeds
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	srand(time(NULL) * rank);

	if (numtasks != NUM_PROCESSES) {
		printf("Expecting %d processors. Terminating.\n", NUM_PROCESSES);
		MPI_Finalize();
		return 0;
	}

	int num_buffers = 0;

	if (is_field_process(rank)) {
		num_buffers = 2 * TEAM_SIZES + 1;
	} else {
		num_buffers = 1;
	}

	players_info = (struct PlayerInfo *) malloc(num_buffers * sizeof (struct PlayerInfo));
	buffers = (int **) malloc(num_buffers * sizeof (int *));
	requests = (MPI_Request *) malloc(num_buffers * sizeof (MPI_Request));
	flat_buffer = (int *) malloc(num_buffers * BUFFER_SIZE * sizeof (int));

	for (i=0; i<num_buffers; i++) {
		buffers[i] = (int *) & flat_buffer[i * BUFFER_SIZE];
	}
	
	init_process(rank, players_info, buffers, ball_position, &all_subfields_comm, &subfield_comm, &report_comm);
	
	MPI_Barrier(MPI_COMM_WORLD);

	printf("Begin match %d \n", rank); 

	int round = 1;
	
	while (round <= 2) {
		if (round == NUM_ROUNDS) {
			is_first_half = false;
		}
		run_process_round(round, rank, players_info, buffers, ball_position, requests, &subfield_comm, &report_comm);
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