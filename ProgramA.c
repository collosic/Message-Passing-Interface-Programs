/*	Christian Collosi
*   11233529 - Lab 2
*   ProgramA.c
*   CS131
*/

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#define MASTER 0
#define MAXLINE 16

/* Globals */
const char NEWLINE[2] = "\n";

FILE* openFile(int argc, char *argv[]) {
	FILE *fptr;
    if (argc == 2) {
    	fptr = fopen(argv[1], "r");
    	if (fptr == NULL) {
        	puts("unable to open file");
        	exit(0);
    	}
	} else {
	    puts("invalid argument");
	    exit(0);
	}
	return fptr; 
}

char* generateData(FILE *fptr, int *NT
							 , int *NS
							 , char *search_str
							 , int *search_array_size) {
    // lets extract the meta data
    char _tempBuf[MAXLINE];
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



    // now lets place the remianing contents in a array 
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

    //dataArray = (char **) malloc(data_size * sizeof(char *));
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
    int rc;
    double t1;
    double t2;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &th_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_th);
    t1 = MPI_Wtime();

    /* The search */
    char *search_array;
	char search_str[MAXLINE];
	int search_array_size = 0;

	/* Array Data infor */
    int NT;
    int NS;
    int slice_size;

    // data that will be printed out
    char found[4] = "no";
 	int location = -1;

   
    if (th_id == MASTER) {
    	// open file if possible
    	FILE *fptr = openFile(argc, argv);

	    // get the data and return it to this char pointer
	    search_array = generateData(fptr, &NT, &NS, search_str, &search_array_size);
	    fclose(fptr);

	    // check and see if the number of processors given is 
	  	// equal to the number provided by the file
	    if (num_th != NT) {
		   printf("You need %d to continue processing\n", NT);
		   MPI_Abort(MPI_COMM_WORLD, rc);
		   exit(0);
		}

		// calculate the slice size for each processor
	    slice_size = search_array_size / NS;    
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
 	char _temp[MAXLINE];

 	// now we can begin the search
 	int i;
 	for (i = 0; i < slice_size; ++i) {
 		// we need to copy over the data MAXLINES at a time
 		strncpy(_temp, sliced_array + (MAXLINE * i), MAXLINE);

 		// check to see if its a match
 		if (!strcmp(_temp, search_str)) {
 			strcpy(found, "yes");
 			location = i + (th_id * slice_size);
 			break;
 		}
 	}

 	// Let's print out the information
 	printf("thread = %d,\tfound %s,\tslice %d,\tposition %d\n", th_id, found, th_id, location);

 	// let's wait for all nodes to finish and print out the time it took
 	MPI_Barrier(MPI_COMM_WORLD);

 	// Make MASTER print out the time
 	if (th_id == MASTER) {
 		t2 = MPI_Wtime();
 		printf("%f\n", t2 - t1);
 		free(search_array);
 	}

 	// free up memory
 	free(sliced_array);

 	// end all nodes
 	MPI_Finalize();
 	return 0;
 }
