#ifndef QUEUE_H
#define QUEUE_H

struct queue_data {
    char* file;
    int socket;
};

typedef struct queue_data* q_data;

struct queue_struct {       // a queue struct, containing capacity, size, front and rear and a char** array that stores the files
    int front, rear;
    
    // size counts the number of files to be inserted
    // capacity is the total MAX size of the queue (given from arguments to the server)
    int size, capacity;           


    q_data* array;       // store files (a file is of char*)
};

typedef struct queue_struct *Queue;

Queue create_queue(int capacity);

int queue_empty(Queue q);

int queue_full(Queue q);

void push(Queue q, char* file, int socket);

q_data pop(Queue q);

#endif // QUEUE_H