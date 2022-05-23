#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

Queue create_queue(int capacity) {

    Queue q = malloc(sizeof(struct queue_st));
    if (q == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    q->capacity = capacity;
    q->front = 0;
    q->size = 0;

    q->rear = capacity-1;

    q->array = (char**)malloc(q->capacity * sizeof(char*));

    return q;
}

void delete_queue(Queue q) {

    for (int i=0; i<q->size; i++) {
        free(q->array[i]);
    }

    free(q);
}

int queue_empty(Queue q) {
    return (q->size == 0);
}

int queue_full(Queue q) {
    return (q->size == q->capacity);
}

void push(Queue q, char* file) {
    
    if (queue_full(q)) {
        return;
    }

    q->rear = (q->rear+1) % q->capacity;
    
    q->array[q->rear] = calloc(strlen(file), sizeof(char));
    strcpy(q->array[q->rear], file);

    q->size = q->size + 1;
}

char* pop(Queue q) {

    if (queue_empty(q)) {
        return NULL;
    }

    char *file = calloc(strlen(q->array[q->front]), sizeof(char));
    strcpy(file, q->array[q->front]);

    q->front = (q->front + 1) % q->capacity;

    q->size--;

    return file;
}