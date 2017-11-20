make

rm -f training.lab.o
rm -f match.lab.o

mpirun -np 12 ./training_mpi >> training.lab.o
mpirun -np 43 ./match_mpi >> match.lab.o