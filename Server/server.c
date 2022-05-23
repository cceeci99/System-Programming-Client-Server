#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "queue.h"

#define BUFFSIZE 4096

Queue queue;

void get_dir_content(char * path);


int main(int argc, char *argv[]) {

    // arguments
    int port, block_sz, queue_sz, pool_sz;

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

    queue = create_queue(queue_sz);

    printf("Server's parameters are:\n");
    printf("Port: %d\n", port);
    printf("Queue size: %d\n", queue_sz);

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int val=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    // specify an address for the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr =  INADDR_ANY;

    // bind socket to specified IP, PORT
    int b = bind(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));
    if (b == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listen for requests
    listen(sock, 5);
    printf("Server Listening...\n");

    while (1) {

        struct sockaddr_in client_addr;
        socklen_t addr_size;

        int client_socket = accept(sock, (struct sockaddr*) &client_addr, &addr_size);
        if (client_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        printf("Client connected\n");

        // bytes to read for the directory string
        int bytes_to_read = 0;
        read(client_socket, &bytes_to_read, sizeof(bytes_to_read));
        bytes_to_read = ntohl(bytes_to_read);

        // read the desired directory's name from client
        char *dirname = calloc(bytes_to_read, sizeof(char));
        read(client_socket, dirname, bytes_to_read);

        // find contents of dir
        get_dir_content(dirname);

        // write the number of files that will be copied to client
        int no_files = htonl(queue->size);
        write(client_socket, &no_files, sizeof(no_files));

        while (!queue_empty(queue)) {

            char *filename = pop(queue);

            // write the number of bytes of the filename to the socket
            int bytes_to_write = htonl(strlen(filename));
            write(client_socket, &bytes_to_write, sizeof(bytes_to_write));

            // write the filename from the queue
            write(client_socket, filename, strlen(filename));
        }
    }

    close(sock);

    return 0;
}

void get_dir_content(char * path) {

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

            printf("Adding file %s to the queue...\n", path_to_file);
            push(queue, path_to_file);
        }
        else if(dir -> d_type == DT_DIR && strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0 ) {    // it's directory

            char subdir[BUFFSIZE];
            sprintf(subdir, "%s/%s", path, dir->d_name);

            // recursively show contents of subdir
            get_dir_content(subdir);
        }
    }
    
    // close dir
    closedir(d);
}