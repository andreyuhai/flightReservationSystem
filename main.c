/*
 * In this project we are creating an array called seats with the size of number of clients
 * which we are getting input by the user. The array is all set to 0 using calloc to mark that all the
 * seats are available in the beginning.
 *
 * Then we are creating two binary semaphore arrays and one
 * binary semaphore. Semaphore arrays (clients, servers) are to be used between server and
 * client pairs and the binary semaphore which is called "s_client" is to be used between clients to avoid
 * race conditions.
 *
 * Each client generates a random seat number and sets "seatNum", then wakes up the corresponding server.
 * Server checks the availability of the seat, if the seat is not occupied then sets "seats[seatNum]" to
 * the clients ID. Then flag_reserved is set to 1 to notify the client that the reservation is complete.
 * After setting the reservation flag, server thread exits waking up the client to let the client thread
 * exit as well.
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
sem_t *clients = NULL;
sem_t *servers = NULL;
sem_t s_client;

// Seat number
int seatNum;
int flag_reserved = 0;

int generateRandSeatNum(int upperLimit) {
    return rand() % (upperLimit);
}

void sleepRandom() {

    // Get a random sleep time between 50 and 200 (inclusive)
    unsigned int sleepTime = 50 + (rand() % 151);

    // Then sleep...zZz
    usleep(sleepTime * 1000);

}

void *client(void *client_id) {


    sem_wait(&s_client);

    int clientID = * (int *)client_id;

    while(1) {

        sem_wait(&clients[clientID]);


        if(!flag_reserved) {

            seatNum = generateRandSeatNum(numberOfSeats);
            //printf("Generated seat num = %d by client %d\n", seatNum, clientID);
            sem_post(&servers[clientID]);

        } else {
            //printf("Exiting thread\n");
            flag_reserved = 0;
            sem_post(&s_client);
            pthread_exit(NULL);
        }
    }

}

void *server(void *client_id) {

    int clientID = *(int *)client_id;

    while(1) {
        sem_wait(&servers[clientID]);

        if(!seats[seatNum]) {
            //printf("Seat number %d reserved for client %d\n", seatNum +1, clientID +1);
            seats[seatNum] = clientID + 1;
            flag_reserved = 1;
            sem_post(&clients[clientID]);
            pthread_exit(NULL);
        }else {
            //printf("Seat was not available\n");
            sem_post(&clients[clientID]);
        }
    }

}

void writeToFile(int *seats, int numSeats){

    FILE *file = fopen("output.txt","w");

    fprintf(file, "Number of total seats :\t%d\n", numSeats);

    fprintf(file, "----------------------------------------------\n");
    for (int i = 0; i < numSeats; i++) {
        fprintf(file, "Client #%d reserves seat #%d\n", seats[i], i);
    }

    fprintf(file, "----------------------------------------------\n");
    fprintf(file, "All seats are reserved\n");
    fclose(file);

}

int main(int argc, char *argv[]) {

    int *thread_IDs = NULL;

    // Getting number of seats from command-line argument.
    numberOfSeats = strtol(argv[1], NULL, 10);

    // Set the seed for rand() to sleep random in milliseconds.
    srand(time(NULL));

    // Creating as many seats as the user defined.
    seats = calloc(numberOfSeats, sizeof(int));

    if(seats == NULL) {
        printf("There was an error callocing for seats\n");
        exit(-1);
    }

    clients = calloc(numberOfSeats, sizeof(sem_t));
    servers = calloc(numberOfSeats, sizeof(sem_t));

    thread_IDs = calloc(numberOfSeats, sizeof(int));

    for (int i = 0; i < numberOfSeats; i++) {
        sem_init(&clients[i], 0, 1);
        sem_init(&servers[i], 0, 0);
        thread_IDs[i] = i;
    }
    sem_init(&s_client, 0, 1);

    pthread_t *clientID = calloc(numberOfSeats, sizeof(pthread_t));
    pthread_t *serverID = calloc(numberOfSeats, sizeof(pthread_t));



    for (int i = 0; i < numberOfSeats ; i++) {
        pthread_create(&clientID[i], NULL, client, &thread_IDs[i]);
        pthread_create(&serverID[i], NULL, server, &thread_IDs[i]);
    }

    for (int i = 0; i < numberOfSeats ; i++) {
        pthread_join(clientID[i], NULL);
        pthread_join(serverID[i], NULL);
    }


    //writeToFile(seats, numberOfSeats);

    printf("\n======================================\n");

    for (int i = 0; i < numberOfSeats; i++) {

        printf("Seat number %d is reserved by client %d\n", i+1, seats[i]);

    }

    // Freeing all the pointers to avoid memory leak.
    free(seats);
    free(thread_IDs);
    free(clientID);
    free(serverID);
    return 0;
}