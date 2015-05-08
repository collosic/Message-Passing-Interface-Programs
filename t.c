/*  Christian Collosi
*   11233529 - Lab 2
*   ProgramA.c
*   CS131
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#define MASTER 0
#define MAXLINE 16
#define SIZE 100000

/* Globals */
const char NEWLINE[2] = "\n";
const char TERM[2] = "\0";

char *s;
char searchStr[MAXLINE];
int NUM_LINES = 0;


char* generateData(FILE *fptr, int *NT, int *NS) {
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
    strcpy(searchStr, _tempBuf);
    strtok(searchStr, NEWLINE);

    // now lets place the remianing contents in a array 
    char s2[MAXLINE];
    while (fgets(s2, MAXLINE, fptr))
        NUM_LINES++;

    // lets now rewind the file back to the beginning
    rewind(fptr);

    // skip the first three lines
    int i;
    for (i = 0; i < 3; ++i)
        fgets(s2, MAXLINE, fptr);

    // now lets allocate our array
    char *t = malloc(NUM_LINES * MAXLINE);
    char *sPtr = t;

    //dataArray = (char **) malloc(data_size * sizeof(char *));
    for (i = 0; i < NUM_LINES; ++i) {
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

    /* Taken from mpi Tutorial given in the assignement */
    int NT;
    int NS;
    int slice_size;

    // data that will be printed out
    char found[4] = "no";
    int location = -1;

    char b[25000][MAXLINE];

    FILE *fptr = NULL;
    if (argc == 2) {
        fptr = fopen(argv[1], "r");
        if (fptr == NULL) {
            puts("unable to open file");
            return -1;
        }
    } else {
        puts("invalid argument");
        return -1;
    } 

    s = generateData(fptr, &NT, &NS);
    fclose(fptr);

    int i;
    char temp[MAXLINE];
    for (i = 0; i < 10; i++) {
        strncpy(temp, s + (MAXLINE * i), MAXLINE);
        printf("%s\n", temp);
    }

    slice_size = NUM_LINES / NS;    

    return 0;
 }
