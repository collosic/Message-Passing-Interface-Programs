/*  Christian Collosi
*   11233529 - Lab 2
*   ProgramB.c
*   CS131
*
*   NOTE: compile and run with the following
*   $ mpicc -o test test.c ; mpirun -np 4 ./test out.txt
*/

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#define MASTER 0
#define BUFFER 256
#define MILLISECONDS 1000

/* Globals */
int th_id;
int num_th;
int tag1 = 1;
int rc;

void my_barrier();
FILE* openFile(char *file_name);

int main(int argc, char *argv[]) {

    double t1;
    double t2;
    double t3;
    FILE *fptr;

    char outFile[BUFFER];

    strcpy(outFile, argv[1]);

    /* Initialize MPI stuff */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &th_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_th);

    t1 = MPI_Wtime();
    my_barrier();
    t2 = MPI_Wtime();

    if (th_id == MASTER) {
        fptr = openFile(outFile);
        t3 = (t2 - t1) * MILLISECONDS;
        fprintf(fptr, "%.3f\n", t3);
        printf("%.3f\n", t3);
        fclose(fptr);
    }
    // end all nodes
    MPI_Finalize();
    return 0;
 }

 void my_barrier() {
    MPI_Status status;
    int num = 42;
    int id;
    int recv, send;

    if (th_id == MASTER) {
        // loop through all the tasks and send
        int i;
        for (i = 1; i < num_th; ++i) {
            MPI_Send(&num, 1, MPI_INT, i, tag1, MPI_COMM_WORLD);
        }

        // loop through and wait for all task id's
        int j;
        for (j = 1; j < num_th; ++j) {
            MPI_Recv(&id, 1, MPI_INT, j, tag1, MPI_COMM_WORLD, &status);
        }
    }

    if (th_id > MASTER) {
        // receive data from MASTER and send back;
        MPI_Recv(&num, 1, MPI_INT, MASTER, tag1, MPI_COMM_WORLD, &status);
        MPI_Send(&th_id, 1, MPI_INT, MASTER, tag1, MPI_COMM_WORLD);
    }

 }

 FILE* openFile(char *file_name) {
    FILE *fptr;
    fptr = fopen(file_name, "w");
    if (fptr == NULL) {
        puts("unable to open file");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(0);
    }
    return fptr; 
}
