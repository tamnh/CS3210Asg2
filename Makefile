all: match training

training: training_mpi.c
	mpicc training_mpi.c -o training_mpi

match: match_mpi.c
	mpicc match_mpi.c -o match_mpi