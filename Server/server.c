#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <errno.h>
#include <sys/socket.h>
#include <dirent.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <sys/time.h>

#include "queue.h"

#define BUFFSIZE 4096

int block_sz;   // global, avoid passing many arguments to threads

Queue queue;    // global shared variable to all threads


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

int mutexes_capacity = 10;      // mutexes init capacity
int mutexes_size = 0;           // mutexes size,  resize (with realloc) when size = capacity
// -------------------------


pthread_mutex_t queue_mutex;        // mutex used for the mutual exclusion over the access of the queue
pthread_cond_t queue_full_cond;     // condition variable, if the queue is full  (communicaction thread waits)
pthread_cond_t queue_empty_cond;    // condition variable, if the queue is empty (worker thread waits)


// read_th is the job which communication thread does
void* read_th(void* args);


// write_th is the job which worker thread does
void* write_th(void* args);


int main(int argc, char *argv[]) {

    // arguments
    int port, queue_sz, pool_sz;

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

    mutexes = malloc(mutexes_capacity*sizeof(client_mutx));
    for (int i=0; i<mutexes_capacity; i++) {
        mutexes[i] = malloc(sizeof(struct client_mutex));
    }

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_empty_cond, NULL);
    pthread_cond_init(&queue_full_cond, NULL);

    pthread_t worker_thread;
    for (int i=0; i<pool_sz; i++) {
        pthread_create(&worker_thread, NULL, &write_th, NULL);
    }

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t addr_size;

        int client_socket = accept(server_socket, (struct sockaddr*) &client_addr, &addr_size);
        if (client_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        printf("Accepted connection from localhost\n");

        // --------------------------------------
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
        pthread_t receiver;
        if (pthread_create(&receiver, NULL, &read_th, &client_socket) == -1) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    close(server_socket);

    return 0;
}


// get directory's contents and push <filename, socket_fd> to the queue
// -------------------------------------------------------------------
void get_dir_content(char *path, int client_socket) {

    // open dir
    DIR * d = opendir(path);
    if (d == NULL) {
        printf("No such dir in Server's hierarchy\n");
        return;
    }

    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {

        if(dir-> d_type != DT_DIR) {                                // if the type is not directory just print it with blue color

            char *path_to_file = calloc(BUFFSIZE, sizeof(char));
            sprintf(path_to_file, "%s/%s", path, dir->d_name);

            // -- Lock mutex to access queue -----
            pthread_mutex_lock(&queue_mutex);

            // wait on condition queue_full, untill some worker thread signals  that there is space available in queue
            if (queue_full(queue)) {
                pthread_cond_wait(&queue_full_cond, &queue_mutex);
            }

            printf("[Thread: %ld]: Adding file %s to the queue...\n", pthread_self(), path_to_file);

            push(queue, path_to_file, client_socket);

            // signal queue_empty condition that there is data in the queue to be poped
            pthread_cond_signal(&queue_empty_cond);

            pthread_mutex_unlock(&queue_mutex);
            // -- Unlock queue mutex , so worker threads can access to it ----
        }
        else if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0) {    // it's directory

            char subdir[BUFFSIZE];
            memset(subdir, 0, BUFFSIZE);
            sprintf(subdir, "%s/%s", path, dir->d_name);

            // recursively show contents of subdir
            get_dir_content(subdir, client_socket);
        }
    }
    
    closedir(d);
}


void count_files(char *path, int* total_files);

void* read_th(void* arg) {      // args: client_socket

    int client_socket = *(int*)arg;

    // 1. Read number of bytes for the directory string
    int bytes_to_read = 0;
    read(client_socket, &bytes_to_read, sizeof(bytes_to_read));
    bytes_to_read = ntohs(bytes_to_read);

    // 2. Read the desired directory from client
    char *dir = calloc(bytes_to_read, sizeof(char));
    read(client_socket, dir, bytes_to_read);

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


void send_file_content(char* file, int client_socket);

void* write_th(void* args) {        // arguments: client_socket, block_sz

    while (1) {     // read continuously untll program is terminated by user
        
        // -- Lock mutex to access queue -----
        pthread_mutex_lock(&queue_mutex);

        // waint on condition queue_empty untill communication thread signals that there is content in the queue
        if (queue_empty(queue)) {
            pthread_cond_wait(&queue_empty_cond, &queue_mutex);
        }
    
        q_data dt = pop(queue);

        // signal queue_full condition because a file was poped from the queue
        pthread_cond_signal(&queue_full_cond);

        pthread_mutex_unlock(&queue_mutex);
        // --- Unlock mutex for queue --------

        char* filename = dt->file;
        int client_socket = dt->socket;

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
    }
}


// Send file's contents to specified client through the socket
// -----------------------------------------------------------
void send_file_content(char* file, int client_socket) {
    
    FILE* fp = fopen(file, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // allocate buffer to read from the file
    char* buff = calloc(block_sz, sizeof(char));
    size_t buff_sz;

    // find the size of the file in bytes
    fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp);

    // reset file pointer to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // 1. Send metadata of file (file_size in bytes)
    int file_sz = htonl(res);
    write(client_socket, &file_sz, sizeof(file_sz));

    // 2. Send Contents of file (it's data)
    while ((buff_sz = fread(buff, sizeof(char), block_sz, fp)) > 0) {      // read block by block, store in buff and return bytes read in buff_sz
        
        // send how many bytes of content the client will read
        int bytes_to_write = htons(buff_sz);
        write(client_socket, &bytes_to_write, sizeof(bytes_to_write));

        // write the buff to the socket 
        write(client_socket, buff, buff_sz);
    }

    free(buff);
}


// count files in given directory and pass total number of files to the integer pointer
void count_files(char *path, int* total_files) {

    DIR * d = opendir(path);
    if (d == NULL) {
        printf("No such dir in Server's hierarchy\n");
        return;
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {

        if(dir-> d_type != DT_DIR) {
            *total_files = *total_files + 1;
        }
        else if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0) {    // it's directory but exclude . and ..
            char subdir[BUFFSIZE];
            sprintf(subdir, "%s/%s", path, dir->d_name);
            count_files(subdir, total_files);
        }
    }
    closedir(d);
}