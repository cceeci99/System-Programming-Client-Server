#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"


// create queue given capacity, return pointer to struct queue_str
Queue create_queue(int capacity) {

    Queue q = malloc(sizeof(struct queue_struct));
    if (q == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    q->capacity = capacity;
    q->front = 0;
    q->size = 0;

    q->rear = capacity-1;

    q->array = malloc(q->capacity*sizeof(q_data));
    if (q->array == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (int i=0; i<q->capacity; i++) {
        
        q->array[i] = malloc(sizeof(struct queue_data));
        if (q->array[i] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }

    return q;
}


// return 1 if queue is empty
int queue_empty(Queue q) {
    return (q->size == 0);
}


// return 1 if queue is full
int queue_full(Queue q) {
    return (q->size == q->capacity);
}


// push new file to given queue
void push(Queue q, char* file, int socket) {
    
    if (queue_full(q)) {
        return;
    }

    q->rear = (q->rear+1) % q->capacity;
    int pos = q->rear;

    q->array[pos]->file =  malloc((strlen(file)+1)*sizeof(char));
    strcpy(q->array[pos]->file, file);

    q->array[pos]->socket = socket;

    q->size = q->size + 1;
}


// pop a file from the queue
q_data pop(Queue q) {

    if (queue_empty(q)) {
        return NULL;
    }

    int pos = q->front;

    q->front = (q->front + 1) % q->capacity;

    q->size--;

    return q->array[pos];
}