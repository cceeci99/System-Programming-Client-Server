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

#include "queue.h"

#define BUFFSIZE 4096

int block_sz;   // global, avoid passing many arguments to threads

Queue queue;    // global shared variable to all threads


pthread_mutex_t work_mutex;
pthread_mutex_t queue_mutex;
pthread_cond_t queue_full_cond;
pthread_cond_t queue_empty_cond;

void* read_th(void* args);
void* write_th(void* args);

// get directory's contents and push <filename, socket_fd> to the queue
void get_dir_content(char *path, int client_socket);

// send file's contents to specified client
void send_file_content(char* file,  int client_socket);


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

    pthread_mutex_init(&work_mutex, NULL);
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


void* read_th(void* arg) {      // args: client_socket

    int client_socket = *(int*)arg;

    // read number of bytes for the directory string
    int bytes_to_read = 0;
    read(client_socket, &bytes_to_read, sizeof(bytes_to_read));
    bytes_to_read = ntohs(bytes_to_read);

    // read the desired directory from client
    char *dirname = calloc(bytes_to_read, sizeof(char));
    read(client_socket, dirname, bytes_to_read);

    printf("[Thread: %ld]: About to scan directory:%s\n", pthread_self(), dirname);

    // find contents of dir
    get_dir_content(dirname, client_socket);

    // write the number of files that will be copied to client
    int no_files = htons(queue->size);
    write(client_socket, &no_files, sizeof(no_files));      // race condition
}


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

            // --------------------------------------------
            pthread_mutex_lock(&queue_mutex);
            pthread_cond_wait(&queue_full_cond, &queue_mutex);

            printf("[Thread: %ld]: Adding file %s to the queue...\n", pthread_self(), path_to_file);

            push(queue, path_to_file, client_socket);
            
            pthread_cond_signal(&queue_empty_cond);
            pthread_mutex_unlock(&queue_mutex);
            // --------------------------------------------
        }
        else if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ) {    // it's directory

            char subdir[BUFFSIZE];
            sprintf(subdir, "%s/%s", path, dir->d_name);

            // recursively show contents of subdir
            get_dir_content(subdir, client_socket);
        }
    }
    
    closedir(d);
}


void* write_th(void* args) {        // arguments: client_socket, block_sz

    while (!queue_empty(queue)) {
        
        // --------------------------------------------
        pthread_mutex_lock(&queue_mutex);
        pthread_cond_wait(&queue_empty_cond, &queue_mutex);

        q_data dt = pop(queue);
        char* filename = dt->file;
        int client_socket = dt->socket;

        printf("[Thread: %ld]: Received task: <%s, %d>\n", pthread_self(), filename, client_socket);

        pthread_cond_signal(&queue_full_cond);
        pthread_mutex_unlock(&queue_mutex);
        // --------------------------------------------

        // write the number of bytes of the filename to the socket
        int bytes_to_write = htons(strlen(filename));
        write(client_socket, &bytes_to_write, sizeof(bytes_to_write));      // race condition

        // write the filename from the queue
        write(client_socket, filename, strlen(filename));                   // race condition

        send_file_content(filename, client_socket);
    }
}


// given file, block_size and the client socket send file's data and contents to the client through socket
// -------------------------------------------------------------------------------------------------------
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

    // 1. Send metadata of file (it's total size in bytes)
    int file_sz = htonl(res);
    write(client_socket, &file_sz, sizeof(file_sz));         // race condition

    // 2. Send contents of file (text data)
    while ((buff_sz = fread(buff, sizeof(char), block_sz, fp)) > 0) {       // read block by block, store in buff and return bytes read in buff_sz
        
        // send how many bytes of content the client will read
        int bytes_to_write = htons(buff_sz);
        write(client_socket, &bytes_to_write, sizeof(bytes_to_write));  // race condition

        // write the buff to the socket 
        write(client_socket, buff, buff_sz);            // race condition
    }
}