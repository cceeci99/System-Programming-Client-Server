#ifndef QUEUE_H
#define QUEUE_H

struct queue_st {
    int front, rear;
    int size;
    int capacity;

    char** array;
};

typedef struct queue_st *Queue;

Queue create_queue(int capacity);

void delete_queue(Queue q);

int queue_empty(Queue q);

int queue_full(Queue q);

void push(Queue q, char* file);

char* pop(Queue q);

#endif // QUEUE_H