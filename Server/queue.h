#ifndef QUEUE_H
#define QUEUE_H

struct queue_st {       // a queue struct, containing capacity, size, front and rear and a char** array that stores the files
    int front, rear;
    int size;           // size counts the number of files to be inserted
    int capacity;       // capacity is the total MAX size of the queue (given from arguments to the server)

    char** array;       // store files (a file is of char*)
};

typedef struct queue_st *Queue;

Queue create_queue(int capacity);

void delete_queue(Queue q);

int queue_empty(Queue q);

int queue_full(Queue q);

void push(Queue q, char* file);

char* pop(Queue q);

#endif // QUEUE_H