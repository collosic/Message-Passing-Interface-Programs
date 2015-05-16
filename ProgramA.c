/*  Christian Collosi
*   11233529 - Lab 2
*   ProgramA.c
*   CS131
*
*   NOTE: compile and run with the following
*   $ mpicc -o test test.c ; mpirun -np 4 ./test in.txt out.txt
*/

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#define MASTER 0
#define MAXLINE 16
#define BUFFER 256
#define MILLISECONDS 1000

/* Globals */
const char NEWLINE[2] = "\n";
int rc;


/* Function used to get a pointer to a file handler */
FILE* openFile(int argc, char *argv[], char *in, char *out) {
    FILE *fptr;
    if (argc > 1) {
        strcpy(in, argv[1]);
        if (argc > 2) strcpy(out, argv[2]);
        fptr = fopen(in, "r");
        if (fptr == NULL) {
            puts("unable to open file");
            MPI_Abort(MPI_COMM_WORLD, rc);
            exit(0);
        }
    } else {
        puts("invalid argument");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(0);
    }
    return fptr; 
}


/* Function to generate the array that will be searched */
char* generateData(FILE *fptr, int *NT
                             , int *NS
                             , char *search_str
                             , int *search_array_size) {

    // lets extract the meta data
    char _tempBuf[MAXLINE];

    // We don't need this data any more I guess
    if(fgets(_tempBuf, MAXLINE, fptr) == NULL) {
       // if we get in here something went wrong with the extraction
       puts("Error extracting data from file");
       exit(-1); 
    }
    *NT = atoi(_tempBuf); 
    if (fgets(_tempBuf, MAXLINE, fptr) == NULL) {
        puts("Error extracting data from file");
        exit(-1);
    }
    *NS = atoi(_tempBuf);
    if (fgets(_tempBuf, MAXLINE, fptr) == NULL) {
        puts("Error extracting data from file");
        exit(-1);
    }

    // here we remove the annoying newline from the matching string
    strtok(_tempBuf, NEWLINE);
    strcpy(search_str, _tempBuf);
    strtok(search_str, NEWLINE);

    // get the size of the array
    char s2[MAXLINE];
    while (fgets(s2, MAXLINE, fptr))
        *search_array_size += 1;

    // lets now rewind the file back to the beginning
    rewind(fptr);

    // skip the first three lines
    int i;
    for (i = 0; i < 3; ++i)
        fgets(s2, MAXLINE, fptr);

    // now lets allocate our array
    char *t = malloc(*search_array_size * MAXLINE);
    char *sPtr = t;

    // populate the array with the data
    for (i = 0; i < *search_array_size; ++i) {
        fgets(s2, MAXLINE, fptr); 
        strtok(s2, NEWLINE);
        strcpy(sPtr, s2);
        sPtr += MAXLINE;
    }
    return t;
}


int main(int argc, char *argv[]) {
    int th_id;
    int num_th;
    double t1;
    double t2;
    double t3;

    char inFile[BUFFER];
    char outFile[BUFFER];

    /* Initialize MPI stuff */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &th_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_th);
    t1 = MPI_Wtime();

    /* The search variables */
    char *search_array;
    char search_str[MAXLINE];
    int search_array_size = 0;

    /* Array Data info */
    int NT;
    int NS;
    int slice_size;

    // data that will be printed out
    char found[4] = "no";
    int slice = -1;
    int location = -1;

    // Have MASTER create the search array
    if (th_id == MASTER) {
        // open file if possible
        FILE *fptr = openFile(argc, argv, inFile, outFile);

        // get the file data and return it to this char pointer
        search_array = generateData(fptr, &NT
                                        , &NS
                                        , search_str
                                        , &search_array_size);
        fclose(fptr);

        // check and see if the number of processors given is 
        // equal to the number provided by the file
        /* This is not needed according to EEE
        if (num_th != NT) {
           printf("You need %d to continue processing\n", NT);
           free(search_array);
           MPI_Abort(MPI_COMM_WORLD, rc);
           exit(0);
        }*/

        // calculate the slice size for each processor
        slice_size = ceil((double) search_array_size / num_th);  
    }

    // Let's send the slice size and the search string to all nodes
    MPI_Bcast(&slice_size, sizeof(int), MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(search_str, MAXLINE, MPI_CHAR, MASTER, MPI_COMM_WORLD);

    // Here we must wait for MASTER to finish reading the information from the file
    MPI_Barrier(MPI_COMM_WORLD);

    // let's allocate memory for the recv side of scatter
    char *sliced_array = malloc(slice_size * MAXLINE);

    // Let's begin the scattering
    MPI_Scatter(search_array, slice_size * MAXLINE
                            , MPI_CHAR, sliced_array
                            , slice_size * MAXLINE
                            , MPI_CHAR, MASTER
                            , MPI_COMM_WORLD);

    // create a temp string so compare with
    char *returnToMaster = malloc(BUFFER);
    char _temp[MAXLINE];
    
    char *masterData = NULL;
    if (th_id == MASTER) 
        masterData = malloc(num_th * BUFFER);

    // now we can begin the search
    int i;
    for (i = 0; i < slice_size; ++i) {
        // we need to copy over the data by MAXLINES at a time
        strncpy(_temp, sliced_array + (MAXLINE * i), MAXLINE);

        // check to see if its a match
        if (!strcmp(_temp, search_str)) {
            strcpy(found, "yes");
            slice = th_id;
            location = i + (th_id * slice_size);
            break;
        }
    }

    // Let's print out the information
    sprintf(returnToMaster, "thread %*d,  found %*s,  slice %*d,  position %*d\n", 3, th_id, 3, found, 3, slice, 5, location);

    // return data back to Master 
    MPI_Gather(returnToMaster, BUFFER
                             , MPI_CHAR
                             , masterData
                             , BUFFER
                             , MPI_CHAR
                             , MASTER
                             , MPI_COMM_WORLD);

    
    // Make MASTER print out master data and the time
    if (th_id == MASTER) {
    	// Check and see if we need to write out to a file
    	FILE *fptr;
    	if (argc > 2)
    		fptr = fopen(outFile, "w");

        char out[BUFFER];
        int i;
        for (i = 0; i < num_th; ++i) {
            strncpy(out, masterData + (BUFFER * i), BUFFER);
            printf("%s", out);
            if (argc > 2) fprintf(fptr, "%s", out);
        }

        // calculate the time and print it out
        t2 = MPI_Wtime();
        t3 = (t2 - t1) * MILLISECONDS;
        printf("%.1f\n", t3);

        if (argc > 2) {
        	fprintf(fptr, "%.1f\n", t3);
        	fclose(fptr);
		}

        // free up the search array 
        free(search_array);
        free(masterData);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // free up memory
    free(returnToMaster);
    free(sliced_array);

    // end all nodes
    MPI_Finalize();
    return 0;
 }
