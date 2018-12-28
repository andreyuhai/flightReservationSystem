/*
 * In this project we are creating an array called seats with the size of number of clients
 * which we are getting input by the user. The array is all set to 0 using calloc to mark that all the
 * seats are available in the beginning.
 *
 * Then we are creating three binary semaphore arrays.
 * Semaphore arrays (s_clients, s_servers) are to be used between server and
 * client pairs and the semaphore array "s_seats" is to be used to lock each seat when a server is
 * checking availability of that specific seat, avoiding all the race conditions concerning seats.
 *
 * Each client generates a random seat number and sets "s_args->requestedSeatNum", then wakes up the corresponding server.
 * Server checks the availability of the seat, if the seat is not occupied then sets "flag_seat_reserved" to
 * 1 to notify the client that the reservation is complete so that client thread can exit as well.
 *
 * If the seat is occupied, then the corresponding server thread basically wakes up the client thread
 * again which will cause the aforementioned process to loop until a seat is finally reserved for the client.
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <semaphore.h>
#include <unistd.h>

// Int array to keep seats. 0 = available, (any-other-number) = occupied.
int *seats = NULL;
int numberOfSeats = 0;

// Semaphore for the so-called "line"
sem_t *s_clients = NULL;
sem_t *s_servers = NULL;
sem_t *s_seats   = NULL;

struct t_arguments {
    int clientID; // Client ID
    int flag_seat_reserved; // Will be set to 1 if a seat reserved for the client
    int requestedSeatNum; // Requested seat number by the client.
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

void writeToFile(char *string){

    FILE *file = fopen("output.txt","a");
    fprintf(file, string);
    fclose(file);

}

void createFile(){

    FILE *file = fopen("output.txt","w");

    fprintf(file, "Number of total seats :\t%d\n", numberOfSeats);
    fprintf(file, "----------------------------------------\n");

    fclose(file);

}

void *client(void *args) {

    struct t_arguments *s_args = args;

    int clientID = s_args->clientID;

    while(1) {

        sem_wait(&s_clients[clientID]);

        if(!s_args->flag_seat_reserved) {

            s_args->requestedSeatNum = generateRandSeatNum(numberOfSeats);
            //printf("Generated seat num = %d by client %d\n", s_args->requestedSeatNum, clientID);
            sem_post(&s_servers[clientID]);

        } else {
            //printf("Exiting thread\n");
            pthread_exit(NULL);
        }
    }

}

void *server(void *args) {

    struct t_arguments *s_args = args;
    char string[50];
    int clientID = s_args->clientID;

    while(1) {
        sem_wait(&s_servers[clientID]);
        sem_wait(&s_seats[s_args->requestedSeatNum]);
        if(!seats[s_args->requestedSeatNum]) {
            //printf("Seat number %d reserved for client %d\n", s_args->requestedSeatNum +1, clientID +1);
            sprintf(string,"Seat number %d reserved for client %d\n", s_args->requestedSeatNum +1, clientID +1);
            writeToFile(string);
            seats[s_args->requestedSeatNum] =  clientID + 1;
            s_args->flag_seat_reserved = 1;
            sem_post(&s_seats[s_args->requestedSeatNum]);
            sem_post(&s_clients[clientID]);
            pthread_exit(NULL);
        }else {
            //printf("Seat was not available\n");
            sem_post(&s_seats[s_args->requestedSeatNum]);
            sem_post(&s_clients[clientID]);
        }
    }
}

int main(int argc, char *argv[]) {

    if(argc < 2 || argc > 2) {
        puts("Usage : ");
        printf("./a.out <number_of_clients>\n");
        exit(-1);
    }

    // Declaring our thread arguments array to pass to each client-server thread pair.
    struct t_arguments *t_args = NULL;

    // Getting number of seats from command-line argument.
    numberOfSeats = strtol(argv[1], NULL, 10);

    if(numberOfSeats > 100 || numberOfSeats < 50) {
        printf("Seats on a plane can not be lower than 50 or higher than 100!\n");
        exit(-1);
    }

    // Set the seed for rand() to sleep random in milliseconds.
    srand(time(NULL));

    // Creating as many seats as the user defined.
    seats = calloc(numberOfSeats, sizeof(int));

    // Checking if calloc was successful.
    if(seats == NULL) {
        printf("There was an error callocing for seats\n");
        exit(-1);
    }

    // Defining semaphores.
    s_clients = calloc(numberOfSeats, sizeof(sem_t));
    s_servers = calloc(numberOfSeats, sizeof(sem_t));
    s_seats   = calloc(numberOfSeats, sizeof(sem_t));

    t_args = calloc(numberOfSeats, sizeof(struct t_arguments));

    // Initializing semaphores and also our thread argument structs.
    for (int i = 0; i < numberOfSeats; i++) {
        sem_init(&s_clients[i], 0, 1);
        sem_init(&s_seats[i], 0, 1);
        sem_init(&s_servers[i], 0, 0);
        t_args[i].clientID = i;
        t_args[i].flag_seat_reserved = 0;
    }

    // Defining clientID and serverID arrays.
    pthread_t *clientID = calloc(numberOfSeats, sizeof(pthread_t));
    pthread_t *serverID = calloc(numberOfSeats, sizeof(pthread_t));

    // Creating file for the first time. Overwriting if it exists.
    createFile();

    // Creating all the threads.
    for (int i = 0; i < numberOfSeats ; i++) {
        pthread_create(&clientID[i], NULL, client, &t_args[i]);
        pthread_create(&serverID[i], NULL, server, &t_args[i]);
    }

    // Waiting for all the threads to exit.
    for (int i = 0; i < numberOfSeats ; i++) {
        pthread_join(clientID[i], NULL);
        pthread_join(serverID[i], NULL);
    }

    writeToFile("----------------------------------------\nAll seats are reserved!");

    // Freeing all the pointers to avoid memory leak.
    free(seats);
    free(t_args);
    free(clientID);
    free(serverID);
    return 0;
}