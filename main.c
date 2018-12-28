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
sem_t *clients = NULL;
sem_t *servers = NULL;
sem_t s_client;

// Seat number
int seatNum;
int seatReserved = 0;

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

    // Then sleep...zZz
    usleep(sleepTime * 1000);

}

void *client(void *args) {

    struct thread_arguments *arguments = args;

    sem_wait(&s_client);

    while(1) {

        sem_wait(&clients[arguments->customerID]);


        if(!seatReserved) {

            seatNum = generateRandSeatNum(arguments->numSeats);
            printf("Generated seat num = %d by client %d\n", seatNum, arguments->customerID);
            sem_post(&servers[arguments->customerID]);

        } else {
            printf("Exiting thread\n");
            seatReserved = 0;
            sem_post(&s_client);
            pthread_exit(NULL);
        }
    }

}

void *server(void *args) {

    struct thread_arguments *arguments = args;

    while(1) {
        sem_wait(&servers[arguments->customerID]);

        if(!seats[seatNum]) {
            printf("Seat number %d reserved for customer %d\n", seatNum, arguments->customerID);
            seatReserved = 1;
            seats[seatNum] = arguments->customerID;
            sem_post(&clients[arguments->customerID]);
            pthread_exit(NULL);
        }else {
            printf("Seat was not available\n");
            sem_post(&clients[arguments->customerID]);
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
        printf("There was an error callocing for seats\n");
        exit(-1);
    }

    // Setting each seat (char actually) to 0 which means available.
    memset(seats, 0, numSeats);

    clients = calloc(numSeats, sizeof(sem_t));
    servers = calloc(numSeats, sizeof(sem_t));

    for (int i = 0; i < numSeats; i++) {
        sem_init(&clients[i], 0, 1);
        sem_init(&servers[i], 0, 0);
    }
    sem_init(&s_client, 0, 1);

    pthread_t *clientID = calloc(numSeats, sizeof(pthread_t));
    pthread_t *serverID = calloc(numSeats, sizeof(pthread_t));

    struct thread_arguments *t_args = calloc(numSeats, sizeof(struct thread_arguments));

    for (int i = 0; i < numSeats; i++) {

        t_args[i].customerID = i;
        t_args[i].numSeats = numSeats;

    }

    for (int i = 0; i < numSeats ; i++) {
        pthread_create(&clientID[i], NULL, client, &t_args[i]);
        pthread_create(&serverID[i], NULL, server, &t_args[i]);
    }

    for (int i = 0; i < numSeats ; i++) {
        pthread_join(clientID[i], NULL);
        pthread_join(serverID[i], NULL);
    }


    puts("---------------");
    for (int i = 0; i < numSeats; i++) {

        printf("Seat num %d reserved for customer num %d\n", i+1, seats[i]+1);

    }

    // Freeing seats at the end of the program to avoid memory leak.
    free(seats);

    return 0;
}