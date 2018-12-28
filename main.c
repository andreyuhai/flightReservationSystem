#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <semaphore.h>
#include <unistd.h>

// Char array to keep seats. 0 = available, (any-other-number) = occupied.
// Using char array, thus the program will take less space on memory.
int *seats = NULL;

// Semaphore for the so-called "line"
sem_t *line = NULL;
sem_t s_server;

// Seat number
int seatNum;
int customerInLine;

// Creating a struct to pass to client threads with the customer ID and number of seats.
struct thread_arguments{
    int customerID;
    int numSeats;
};

int generateRandSeatNum(int upperLimit) {
    return rand() % (upperLimit);
}

void sleepRandom() {

    // Get a random sleep time between 50 and 200 (inclusive)
    unsigned int sleepTime = 50 + (rand() % 151);

    printf("Sleeping %d milliseconds", sleepTime);

    // Then sleep...zZz
    usleep(sleepTime * 1000);

}

void client(void *args) {

    struct thread_arguments *arguments = args;

    sleepRandom();


    while(1) {

        sem_wait(&line[arguments->customerID]);

        if(customerInLine != arguments->customerID) {

            printf("Exiting thread");
            pthread_exit(NULL);
        }

        seatNum = generateRandSeatNum(arguments->numSeats);

        sem_post(&s_server);

    }

}

void server(void *args) {

    struct thread_arguments *arguments = args;

    sem_post(&line[arguments->customerID]);

    sem_wait(&s_server);


    while(1) {
        // If the seat is available.
        if(!seatNum) {

            seats[seatNum] = arguments->customerID;
            printf("Seat number : %d is reserved for client : %d\n", seatNum+1, arguments->customerID+1);
            customerInLine = -1;
            sem_post(&line[customerInLine]);
            sem_post(&s_server);
            pthread_exit(NULL);

        } else { // If the seat is not available

            sem_post(&line[customerInLine]);
            printf("Seat was not available");

        }

    }
}

int main(int argc, char *argv[]) {

    // Getting number of seats from command-line argument.
    int numSeats = strtol(argv[1], NULL, 10);

    // Set the seed for rand() to sleep random in milliseconds.
    srand(time(NULL));

    // Creating as many seats as the user defined.
    seats = calloc(numSeats, sizeof(int));

    if(seats == NULL) {
        printf("There was an error callocing for seats");
        exit(-1);
    }

    // Setting each seat (char actually) to 0 which means available.
    memset(seats, 0, numSeats);

    // Create as many semaphores as there are clients in the line.
    line = calloc(numSeats, sizeof(sem_t));

    for(int i = 0; i < numSeats; i++) {
        sem_init(&line[i], 0, 0); // Initializing semaphores for the line.
    }

    // Initializing semaphore for the servers.
    sem_init(&s_server, 0, 0);
    


    // Freeing seats at the end of the program to avoid memory leak.
    free(seats);

    return 0;
}