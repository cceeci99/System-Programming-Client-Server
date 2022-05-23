#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFSIZE 4096

int main(int argc, char *argv[]) {

    // arguments
    char serverIP[40];
    char *dir = calloc(BUFFSIZE, sizeof(char));
    int port;

    for (int i=1; i<argc; i++) {

        if (strcmp(argv[i-1], "-p") == 0) {
            port = atoi(argv[i]);
        }
        if (strcmp(argv[i-1], "-i") == 0) {
            strcpy(serverIP, argv[i]);
        }
        if (strcmp(argv[i-1], "-d") == 0) {
            strcpy(dir, argv[i]);
        }
    }
    
    printf("Client's parameters are:\n");

    printf("Port: %d\n", port);
    printf("Server's IP: %s\n", serverIP);
    printf("Directory: %s\n", dir);

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    int val=1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(serverIP);

    // connect to socket
    int connection = connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr));

    if (connection == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Client connected to server, desired dir to copy: Server/%s\n", dir);
    
    int bytes_to_write = htonl(strlen(dir));
    write(sock, &bytes_to_write, sizeof(bytes_to_write));

    write(sock, dir, strlen(dir));

    // read the number of files that are gonna be copied
    int no_files = 0;
    read(sock, &no_files, sizeof(no_files));
    no_files = ntohl(no_files);

    printf("%d files to copy\n", no_files);

    for (int i=0; i<no_files; i++) {

        // read the number of byte for the filename
        int bytes_to_read = 0;
        read(sock, &bytes_to_read, sizeof(bytes_to_read));
        bytes_to_read = ntohl(bytes_to_read);

        char* filename = calloc(bytes_to_read, sizeof(char));
        read(sock, filename, bytes_to_read);

        printf("Received file: %s\n", filename);
    }

    close(sock);

    return 0;
}