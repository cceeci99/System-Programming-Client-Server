#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "thread_jobs.h"

// in this header file, are declared all global variables that are used for the synchronizing of the threads
// queue, mutexes, sizes etc... Must check it before reading the dataServer.c
#include "glob_vars.h"


// read_th is the job which communication thread does
void* communication_thread_t(void* args);


// write_th is the job which worker thread does
void* worker_thread_t(void* args);


int main(int argc, char *argv[]) {

    // arguments
    int port, queue_sz, pool_sz;    // block_sz is global variable

    for (int i=1; i<argc; i++) {
        
        if (strcmp(argv[i-1], "-p") == 0) {
            port  = atoi(argv[i]);
        }
        if (strcmp(argv[i-1], "-b") ==0) {
            block_sz = atoi(argv[i]);
        }
        if (strcmp(argv[i-1], "-q") == 0) {
            queue_sz = atoi(argv[i]);
        }
        if (strcmp(argv[i-1], "-s") == 0) {
            pool_sz = atoi(argv[i]);
        }
    }

    printf("Server's parameters are:\n");
    // ----------------------------------
    printf("Port: %d\n", port);
    printf("Queue size: %d\n", queue_sz);
    printf("Block size: %d\n", block_sz);
    printf("Thread pool size: %d\n", pool_sz);

    // queue declared in glob_vars.h
    queue = create_queue(queue_sz);

    // create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    int val=1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    // specify an address for the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr =  INADDR_ANY;

    // bind socket to specified IP, PORT
    int b = bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if (b == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen for requests
    listen(server_socket, 5);
    printf("Listening for connection on port: %d...\n", port);

    // initializing mutexes struct 
    mutexes_capacity = INIT_MUTEXES_CAPACITY;
    mutexes_size = 0;

    mutexes = malloc(mutexes_capacity*sizeof(client_mutx));
    for (int i=0; i<mutexes_capacity; i++) {
        mutexes[i] = malloc(sizeof(struct client_mutex));
    }

    // initializing mutexes and cond_vars
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_empty_cond, NULL);
    pthread_cond_init(&queue_full_cond, NULL);

    // Create Thread Pool 
    pthread_t worker_thread;
    for (int i=0; i<pool_sz; i++) {
        pthread_create(&worker_thread, NULL, &worker_thread_t, NULL);
    }

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t addr_size;

        int client_socket = accept(server_socket, (struct sockaddr*) &client_addr, &addr_size);
        if (client_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        printf("Accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));

        // ---------------------------------------
        if (mutexes_size >= mutexes_capacity) {     // resize if mutexes_array size has reached capacity
            mutexes_capacity *= 2;
            mutexes = realloc(mutexes, mutexes_capacity*sizeof(client_mutx));

            for (int i=mutexes_size; i<mutexes_capacity; i++) {

                mutexes[i] = malloc(sizeof(struct client_mutex));

                if (mutexes[i] == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // add new mutex to the corresponding socket
        mutexes[mutexes_size]->socket = client_socket;
        pthread_mutex_init(&(mutexes[mutexes_size]->mutex), NULL);

        mutexes_size++;
        // --------------------------------------

        // create a communication thread
        pthread_t receiver_thread;
        if (pthread_create(&receiver_thread, NULL, &communication_thread_t, &client_socket) == -1) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    close(server_socket);

    return 0;
}


// communication thread
void* communication_thread_t(void* arg) {      // args: client_socket

    int client_socket = *(int*)arg;

    // 1. Read number of bytes for the directory string
    int bytes_to_read = 0;
    read(client_socket, &bytes_to_read, sizeof(bytes_to_read));
    bytes_to_read = ntohs(bytes_to_read);

    // 2. Read the desired directory from client
    char *dir = malloc(bytes_to_read+1);
    read(client_socket, dir, bytes_to_read);
    dir[bytes_to_read] = '\0';

    printf("[Thread: %ld]: About to scan directory:%s\n", pthread_self(), dir);

    // 3. Count how many files in the given directory will be copied
    int total_files = 0;
    count_files(dir, &total_files);
    
    // 4. Write the number of files that will be copied to the socket
    int no_files = htons(total_files);
    write(client_socket, &no_files, sizeof(no_files));

    // 5. Find contents of dir and push to the queue  <File, Socket_fd>
    get_dir_content(dir, client_socket);

    free(dir);
}


// worker thread
void* worker_thread_t(void* args) {

    while (1) {     // read continuously untll program is terminated by user
        
        // -- Lock mutex to access queue -----
        pthread_mutex_lock(&queue_mutex);

        // wait on condition queue_empty untill communication thread signals that there is content in the queue
        while (queue_empty(queue)) {
            pthread_cond_wait(&queue_empty_cond, &queue_mutex);
        }
    
        q_data dt = pop(queue);

        // signal queue_full condition because a file was poped from the queue
        pthread_cond_signal(&queue_full_cond);

        pthread_mutex_unlock(&queue_mutex);
        // --- Unlock mutex for queue --------

        int client_socket = dt->socket;

        char *filename = malloc((strlen(dt->file) + 1)*sizeof(char));
        strcpy(filename, dt->file);

        printf("[Thread: %ld]: Received task: <%s, %d>\n", pthread_self(), filename, client_socket);

        // find the corresponding mutex for this socket
        int i;
        for (i=0; i<mutexes_size; i++) {
            if (mutexes[i]->socket == client_socket) {
                break;
            }
        }

        // -- Lock mutex for this client, to send file's data ----
        pthread_mutex_lock(&mutexes[i]->mutex);

        // 1. send number of bytes for the filename
        int bytes_to_write = htons(strlen(filename));
        write(client_socket, &bytes_to_write, sizeof(bytes_to_write));      // race condition

        // 2. send the filename
        write(client_socket, filename, strlen(filename));                   // race condition

        // 3. send file contents
        send_file_content(filename, client_socket);

        pthread_mutex_unlock(&mutexes[i]->mutex);
        // -- Unlock mutex for this client so others can write too ----

        free(filename);
    }
}