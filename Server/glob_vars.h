#ifndef GLOB_VARS_H
#define GLOB_VARS_H

#define BUFFSIZE 4096
#define INIT_MUTEXES_CAPACITY 10

#include "queue.h"


Queue queue;    // global shared variable to all threads

int block_sz;

// A 'map' of  (sockets, mutexes) is needed for the mutual exclusion over the socket on which workers will write
// Two workers can't write at the same time on the same socket, but when one thread is writing on other socket there can be others workers writing on other sockets

// -----------------------
struct client_mutex {
    pthread_mutex_t mutex;
    int socket;
};

typedef struct client_mutex *client_mutx;

// dynamic array for mutexes
client_mutx *mutexes;

int mutexes_capacity;      // mutexes init capacity
int mutexes_size;           // mutexes size,  resize (with realloc) when size = capacity
// -------------------------


pthread_mutex_t queue_mutex;        // mutex used for the mutual exclusion over the access of the queue
pthread_cond_t queue_full_cond;     // condition variable, if the queue is full  (communicaction thread waits)
pthread_cond_t queue_empty_cond;    // condition variable, if the queue is empty (worker thread waits)


#endif //GLOB_VARS_H