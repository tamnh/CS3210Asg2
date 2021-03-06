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
const int BUFFER_SIZE = 8;
const int NUM_PROCESSES = 34;

const int TEAM_A_FIRST_ID = 12;
const int TEAM_A_LAST_ID = 22;
const int TEAM_B_FIRST_ID = 23;
const int TEAM_B_LAST_ID = 33;

const int GOAL_RANGE[2] = {43, 51};

const int ATTRIBUTE_LOWER_BOUND = 2;

//unique key to create communication channels
const int PLAYERS_COMM_KEY = 100;


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
	int id;
	int prev_position[2];
	int current_position[2];
	int reached_ball_this_round;
	int win_ball_this_round;
	int ball_challenge;
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


void assign_random_attributes(struct PlayerInfo * player_info) {
	//from 2 to 8
	int range = 6;
	int dribbling_skill = 2 + rand() % (range + 1);
	range = min(range, 15 - dribbling_skill - 4);
	int kick_power = 2 + rand() % (range + 1);
	int speed_skill = 15 - dribbling_skill - kick_power;

	player_info->kick_power = kick_power;
	player_info->speed_skill = speed_skill;
	player_info->dribbling_skill = dribbling_skill;
}

//helper functions
void print_player_info(struct PlayerInfo * player_info) {
	printf("%d %d %d %d %d %d %d %d\n", player_info->id, player_info->prev_position[0], player_info->prev_position[1], player_info->current_position[0], player_info->current_position[1], 
		player_info->reached_ball_this_round, player_info->win_ball_this_round, player_info->ball_challenge);
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
	position[0] = rand() % FIELD_SIZES[0] ;
	position[1] = rand() % FIELD_SIZES[1];
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
	
	// player_info->num_meters_run = 0;
	// player_info->num_times_reached_ball = 0;
	// player_info->num_times_won_ball = 0;
	player_info->win_ball_this_round = 0;
	player_info->reached_ball_this_round = 0;

	/*
	int attr_idx = rank - NUM_FIELD_PROCESS;
	player_info->speed_skill = ATTRIBUTES_LIST[attr_idx][0];
	player_info->dribbling_skill = ATTRIBUTES_LIST[attr_idx][1];
	player_info->kick_power = ATTRIBUTES_LIST[attr_idx][2];
	*/

	assign_random_attributes(player_info);
	//printf("%d %d %d %d\n", rank, player_info->dribbling_skill, player_info->speed_skill, player_info->kick_power);

	//get player id 
	if (is_team_A_player(rank)) {
		player_info->id = rank - TEAM_A_FIRST_ID;
	} else {
		player_info->id = rank - TEAM_B_FIRST_ID;
	}
}


void reset_player_position(struct PlayerInfo * player_info) {
	init_random_position(player_info->current_position);
}


void init_player_process(int rank, struct PlayerInfo * player_info, MPI_Comm * all_subfields_comm, MPI_Comm * players_comm) {
	int tag = 0;
	init_player(player_info, rank);

	MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, rank, all_subfields_comm);

	MPI_Comm_split(MPI_COMM_WORLD, PLAYERS_COMM_KEY, rank, players_comm);
	// print_player_info(player_info);
}


void init_field_process(int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Comm * all_subfields_comm, MPI_Comm * players_comm) {
	int i;
	int tag = 0;

	MPI_Comm_split(MPI_COMM_WORLD, 0, rank, all_subfields_comm);

	if (rank == 0) {
		init_random_position(ball_position);
		MPI_Comm_split(MPI_COMM_WORLD, PLAYERS_COMM_KEY, rank, players_comm);
	} else {
		MPI_Comm_split(MPI_COMM_WORLD, rank, rank, players_comm);
	}
}


void init_process(int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Comm * all_subfields_comm, MPI_Comm * subfield_comm, MPI_Comm * players_comm) {
	if (is_field_process(rank)) {
		init_field_process(rank, player_info, buffers, ball_position, all_subfields_comm, players_comm);
	} else {
		init_player_process(rank, &player_info[0], all_subfields_comm, players_comm);
	}
}


void load_player_info_from_buffer(int * buffer, struct PlayerInfo * player_info) {
	player_info->prev_position[0] = buffer[0];
	player_info->prev_position[1] = buffer[1];
	player_info->current_position[0] = buffer[2];
	player_info->current_position[1] = buffer[3];
	player_info->reached_ball_this_round = buffer[4];
	player_info->win_ball_this_round = buffer[5];
	player_info->ball_challenge = buffer[6];
	player_info->id = buffer[7];
}


void load_player_info_into_buffer(int * buffer, struct PlayerInfo * player_info) {
	buffer[0] = player_info->prev_position[0];
	buffer[1] = player_info->prev_position[1];
	buffer[2] = player_info->current_position[0];
	buffer[3] = player_info->current_position[1];
	buffer[4] = player_info->reached_ball_this_round;
	buffer[5] = player_info->win_ball_this_round;
	buffer[6] = player_info->ball_challenge;
	buffer[7] = player_info->id;
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
void move_to_point(int * src, int * dest, int max_run) {
	//the directed distance
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


void send_player_info_to_field_process(int * buffer, struct PlayerInfo * player_info, MPI_Comm * players_comm) {
	load_player_info_into_buffer(buffer, player_info);
	MPI_Gather(buffer, BUFFER_SIZE, MPI_INT, NULL, BUFFER_SIZE, MPI_INT, 0, *players_comm);
	MPI_Barrier(* players_comm);
}


void perform_kick(int rank, struct PlayerInfo * player_info, int * ball_position) {
	int target_goal[2];
	target_goal[0] = 0;

	if ((is_team_A_player(rank) && is_first_half) || (is_team_B_player(rank) && !is_first_half)) {
		target_goal[0] = FIELD_SIZES[0] - 1;
	}

	target_goal[1] = player_info->current_position[1];
	target_goal[1] = GOAL_RANGE[0] + rand() % (GOAL_RANGE[1] - GOAL_RANGE[0] + 1);	
	move_to_point(ball_position, target_goal, 2 * player_info->kick_power);
}


void join_challenge_phase(int rank, int field_id, struct PlayerInfo * player_info, int * buffer, MPI_Comm * subfield_comm, int * ball_position, int * winner) {
	int i;
	int ball_challenge = -1;
	
	for (i=0; i<BUFFER_SIZE; i++) {
		buffer[i] = 0;
	}

	//set some parameters;
	buffer[0] = rank;

	if (is_same_position(player_info->current_position, ball_position)) {
		//update reaching count
		player_info->reached_ball_this_round = 1;
		
		ball_challenge = 1 + rand() % 9;
		ball_challenge *= player_info->dribbling_skill;
		//load the buffer
		buffer[1] = ball_challenge;
		perform_kick(rank, player_info, ball_position);
		buffer[2] = ball_position[0];
		buffer[3] = ball_position[1];
	}

	player_info->ball_challenge = ball_challenge;
	
	MPI_Gather(buffer, BUFFER_SIZE, MPI_INT, NULL, BUFFER_SIZE, MPI_INT, 0, *subfield_comm);

	MPI_Barrier(*subfield_comm);

	MPI_Bcast(winner, 1, MPI_INT, field_id, *subfield_comm);

	if ((*winner) == rank) {
		player_info->win_ball_this_round = 1;
	}
}


void run_player_round(int rank, struct PlayerInfo * player_info, int * buffer, int * ball_position, MPI_Comm * subfield_comm, MPI_Comm * players_comm, int * reset, int * winner) {
	//sync ball position
	if (reset[0] > 0) {
		reset_player_position(player_info);
		reset[0] = 0;
	} 

	//reset parameters
	player_info->win_ball_this_round = 0;
	player_info->reached_ball_this_round = 0;
	player_info->ball_challenge = -1;

	MPI_Bcast(ball_position, 2, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	
	//move all player to the ball
	player_info->prev_position[0] = player_info->current_position[0];
	player_info->prev_position[1] = player_info->current_position[1];

	if (rand() % 2) {
		move_to_point(player_info->current_position, ball_position, min(player_info->speed_skill, MAX_RUN));
	}

	//update distance run
	//player_info->num_meters_run += get_distance(player_info->prev_position, player_info->current_position);

	int field_id = get_field_index(player_info->current_position);
	MPI_Comm_split(MPI_COMM_WORLD, field_id, rank, subfield_comm);
	
	if (field_id == get_field_index(ball_position)) {
		join_challenge_phase(rank, field_id, player_info, buffer, subfield_comm, ball_position, winner);
	}

	MPI_Comm_free(subfield_comm);
	// printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
	send_player_info_to_field_process(buffer, player_info, players_comm);
	MPI_Bcast(reset, 1, MPI_INT, 0, *players_comm);
}


void handle_field_challenges(int rank, int ** buffers, MPI_Comm * subfield_comm, int * winner, int * ball_position) {
	int i;
	int * dummy_buffer = (int *) malloc (BUFFER_SIZE * sizeof (int));

	MPI_Gather(dummy_buffer, BUFFER_SIZE, MPI_INT, buffers[0], BUFFER_SIZE, MPI_INT, 0, *subfield_comm);
	MPI_Barrier(* subfield_comm);

	* winner = -1;
	//get the winner;
	int no_participants;
	int max_challenge = 0;
	int max_challenge_count = 0;
	int winner_id, cnt;

	MPI_Comm_size(*subfield_comm, &no_participants);

	if (no_participants > 1) {
		//loop though
		for (i=1; i<no_participants; i++) {
			if (buffers[i][1] >= max_challenge) {
				if (buffers[i][1] == max_challenge) {
					max_challenge_count ++;
				} else {
					max_challenge = buffers[i][1];
					max_challenge_count = 1;
				}
			}
		}

		if (max_challenge > 0) {
			winner_id = rand() % max_challenge_count;
			cnt=0;
			for (i=1; i<no_participants; i++) {
				if (buffers[i][1] == max_challenge) {
					if (winner_id == cnt) {
						//setting winner and choose the new ball position
						* winner = buffers[i][0];
						ball_position[0] = buffers[i][2];
						ball_position[1] = buffers[i][3];
					}

					cnt++;
				} 
			}
		}
	}
	
	MPI_Bcast(winner, 1, MPI_INT, rank, *subfield_comm);

	free(dummy_buffer);
}


void receive_players_info(int ** buffers, struct PlayerInfo * players_info, MPI_Comm * players_comm) {
	int i;
	int * dummy_buffer = (int *) malloc (BUFFER_SIZE * sizeof (int));
	
	MPI_Gather(dummy_buffer, BUFFER_SIZE, MPI_INT, buffers[0], BUFFER_SIZE, MPI_INT, 0, *players_comm);
	MPI_Barrier(* players_comm);

	for (i=1; i<=2*TEAM_SIZES; i++) {
		load_player_info_from_buffer(buffers[i], &players_info[i]);
	}

	free(dummy_buffer);
}


void check_ball_position(int * ball_position, int * reset, int * score) {
	* reset = 0;

	if (ball_position[1] <= GOAL_RANGE[1] && ball_position[1] >= GOAL_RANGE[0]) {
		if ((ball_position[0] == 0 && is_first_half) && (ball_position[0]==FIELD_SIZES[0]-1 && !is_first_half)) {
			score[1]++;
			* reset = 1;
		} else if ((ball_position[0] == 0 && !is_first_half) && (ball_position[0]==FIELD_SIZES[0]-1 && is_first_half)){
			score[0]++;
			* reset = 1;
		}

	}
}


void run_field_round(int round, int rank, struct PlayerInfo * players_info, int ** buffers, int * ball_position, MPI_Comm * all_subfields_comm,
	MPI_Comm * subfield_comm, MPI_Comm * players_comm, int * reset, int * winner, int * score) {
	int i;
	int field_id = get_field_index(ball_position);

	if (rank == 0) {
		//we don't store the position of the ball
		printf("%d\n", round);
		printf("%d %d\n", ball_position[0], ball_position[1]);
	}

	MPI_Bcast(ball_position, 2, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	// printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
	MPI_Comm_split(MPI_COMM_WORLD, rank, rank, subfield_comm);

	if (field_id == rank) {
		handle_field_challenges(rank, buffers, subfield_comm, winner, ball_position);
	}


	MPI_Comm_free(subfield_comm);

	MPI_Bcast(ball_position, 2, MPI_INT, field_id, *all_subfields_comm);
	MPI_Barrier(*all_subfields_comm);

	//to be factored
	if (rank == 0) {

		receive_players_info(buffers, players_info, players_comm);
		for (i=1; i<=2*TEAM_SIZES; i++) {
			if (i==1) printf("TEAM A: \n");
			if (i==TEAM_SIZES+1) printf("TEAM B: \n");
			print_player_info(&players_info[i]);
		}
		

		check_ball_position(ball_position, reset, score);

		MPI_Bcast(reset, 1, MPI_INT, 0, *players_comm);

		//reset ball position
		if (*reset == 1) {
			ball_position[0] = 64;
			ball_position[1] = 48;
		}

		printf("Current score: %d %d\n", score[0], score[1]);
	}
}


void run_process_round(int round, int rank, struct PlayerInfo * player_info, int ** buffers, int * ball_position, MPI_Comm * all_subfields_comm,
	MPI_Comm * subfield_comm, MPI_Comm * players_comm, int * reset, int * winner, int * score) {
	if (is_field_process(rank)) {
		//printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
		run_field_round(round, rank, player_info, buffers, ball_position, all_subfields_comm, subfield_comm, players_comm, reset, winner, score);
	} else {
		// printf("%d %d %d\n", rank, ball_position[0], ball_position[1]);
		run_player_round(rank, &player_info[0], buffers[0], ball_position, subfield_comm, players_comm, reset, winner);
	}
}


int main(int argc,char *argv[])
{
	int numtasks, rank;
	int i;

	struct PlayerInfo * players_info;

	int ** buffers;
	int * flat_buffer;
	int ball_position[2];
	
	//check whether we need to reset the position of the player 
	int reset = 0;
	int winner = -1;
	int score[2]; score[0] = 0; score[1] = 0;

	MPI_Comm all_subfields_comm;
	MPI_Comm subfield_comm;
	MPI_Comm players_comm;

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
	flat_buffer = (int *) malloc(num_buffers * BUFFER_SIZE * sizeof (int));

	for (i=0; i<num_buffers; i++) {
		buffers[i] = (int *) & flat_buffer[i * BUFFER_SIZE];
	}
	
	init_process(rank, players_info, buffers, ball_position, &all_subfields_comm, &subfield_comm, &players_comm);
	
	MPI_Barrier(MPI_COMM_WORLD);

	int round = 1;
	
	while (round <= 2 * NUM_ROUNDS) {
		
		if (round == NUM_ROUNDS + 1) {
			is_first_half = false;
			reset = 1;
		}

		run_process_round(round, rank, players_info, buffers, ball_position, &all_subfields_comm, &subfield_comm, &players_comm, &reset, &winner, score);
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

	free(flat_buffer);
	free(buffers);
	free(players_info);

	return 0;
}