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

            char *path_to_file = malloc(BUFFSIZE*sizeof(char));
            memset(path_to_file, 0, BUFFSIZE);

            sprintf(path_to_file, "%s/%s", path, dir->d_name);

            // -- Lock mutex to access queue -----
            pthread_mutex_lock(&queue_mutex);

            // wait on condition queue_full, untill some worker thread signals  that there is space available in queue
            while (queue_full(queue)) {
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



// Send file's contents to specified client through the socket
// -----------------------------------------------------------
void send_file_content(char* file, int client_socket) {
    
    FILE* fp = fopen(file, "rb");
    if (fp == NULL) {
        strerror(errno);
        exit(EXIT_FAILURE);
    }

    // allocate buffer to read from the file
    char *buff = malloc(block_sz*sizeof(char));
    memset(buff, 0, block_sz);

    // find the size of the file in bytes
    fseek(fp, 0L, SEEK_END);
    long int file_sz = ftell(fp);

    // reset file pointer to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // 1. Send metadata of file (file_size in bytes)
    uint32_t file_sz_t = htonl((uint32_t) file_sz);
    write(client_socket, &file_sz_t, sizeof(file_sz_t));

    uint16_t block_sz_t = htons((uint16_t) block_sz);
    write(client_socket, &block_sz_t, sizeof(block_sz_t));

    int total_bytes_sent = 0;

    // 2. Send Contents of file (it's data)
    while (total_bytes_sent < file_sz) {
        
        memset(buff, 0, block_sz);
        
        fread(buff, block_sz, 1, fp);       // read 1 element with block_sz size and store it in buff

        int bytes_written = 0;
        if (file_sz - total_bytes_sent < block_sz) {
            bytes_written = write(client_socket, buff, file_sz - total_bytes_sent);
        }
        else {
            bytes_written = write(client_socket, buff, block_sz);
        }

        total_bytes_sent = total_bytes_sent + bytes_written;
    }
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
            memset(subdir, 0, BUFFSIZE);
            sprintf(subdir, "%s/%s", path, dir->d_name);
            count_files(subdir, total_files);
        }
    }

    closedir(d);
}