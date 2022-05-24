#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFSIZE 4096

int create_dir(char *name);
FILE* create_file(char *name);
void copy_file(FILE* fp, int socket);

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
    printf("Server's IP: %s\n", serverIP);
    printf("Port: %d\n", port);
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
    
    printf("Connecting to %s on port %d\n", serverIP, port);
    
    // send to server how many bytes he will read for the directory name
    int bytes_to_write = htons(strlen(dir));
    write(sock, &bytes_to_write, sizeof(bytes_to_write));

    // Write the desired directory to copy, into the socket 
    write(sock, dir, strlen(dir));

    // read the number of files that are gonna be copied
    int no_files = 0;
    read(sock, &no_files, sizeof(no_files));
    no_files = ntohs(no_files);

    printf("no_files to copy: %d\n", no_files);

    // Receiving Files ...
    // --------------------
    for (int i=0; i<no_files; i++) {

        // 1. read the number of byte for the filename
        int bytes_to_read = 0;
        read(sock, &bytes_to_read, sizeof(bytes_to_read));
        bytes_to_read = ntohs(bytes_to_read);

        // 2. Receive the filename (relative path)
        char* filename = calloc(bytes_to_read, sizeof(char));
        read(sock, filename, bytes_to_read);

        printf("Received file: %s\n", filename);

        // 3. Create needed directory hierarchy
        char* temp = strdup(filename);
        char* token = strtok(temp, "/");
        char* dir = malloc(strlen(token));
        char* last = NULL;

        while (token != NULL) {
            
            if (last != NULL) {

                // construct each directory from the hierarchy and create it
                dir = realloc(dir, strlen(dir) + strlen(last) + 1);
                strcat(dir, last);
                strcat(dir, "/");

                create_dir(dir);
            }

            last = token;
            token = strtok(NULL, "/");
        }
        
        // 4. create file
        FILE* fp = create_file(filename);
        
        // 5. copy contents to the file
        copy_file(fp, sock);
    }

    close(sock);

    return 0;
}



// copy contents of file that are passed from server through the socket to local file with FILE pointer fp
// -------------------------------------------------------------------------------------------------------
void copy_file(FILE* fp, int socket) {

    // 1. Receiving metadata of file (File_size)

    // read the file size in bytes that are supposed to be copied
    int file_sz = 0;
    read(socket, &file_sz, sizeof(file_sz));
    file_sz = ntohs(file_sz);

    printf("File size in bytes to copy: %d\n", file_sz);

    int total_bytes_copied = 0;

    // 2. Receiving contents of file
    while (total_bytes_copied < file_sz) {          // stop copying contents when all bytes are copied
                                                    // with this way server can know how many times he will read from socket
        // read number of bytes per block to read
        int block_bytes = 0;
        read(socket, &block_bytes, sizeof(block_bytes));
        block_bytes = ntohs(block_bytes);

        // contents of block will be stored in variable block
        char* block = calloc(block_bytes, sizeof(char));
        read(socket, block, block_bytes);

        // printf("Client read %d bytes, block: %s\n", block_bytes, block);
        
        // write all the bytes of block into the file
        fwrite(block, sizeof(char), block_bytes, fp);

        total_bytes_copied += block_bytes;
    }

    printf("Total bytes copied: %d\n", total_bytes_copied);
}



// return 1 if given file exists or 0
// ----------------------------------
int file_exists(char *filename) {
    
    struct stat   buffer;   
    return (stat (filename, &buffer) == 0);
}



// create directory with given name
// --------------------------------
int create_dir(char *name) {
    
    // remove the last '/' character from the name  e.x  bar/foo/foobar/ 
    char* dir = malloc(strlen(name)-1);
    memcpy(dir, name, strlen(name)-1);

    if (mkdir(dir, S_IRWXU) != 0 && errno != EEXIST) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }

    return 0;
}



// create/open file with given name and return FILE pointer
// -------------------------------------------------------
FILE* create_file(char *name) {

    if (file_exists(name)) {        // if it exists delete it and create new one
        unlink(name);
    }

    FILE* fp = fopen(name, "w");    // creates and open file with write mode
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    return fp;                      // return file pointer
}