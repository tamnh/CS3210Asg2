make

rm -f training.lab.o
rm -f match.lab.o

time mpirun -np 12 ./training_mpi >> training.lab.o
time mpirun -np 34 ./match_mpi >> match.lab.o
