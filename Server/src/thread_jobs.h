#ifndef THREAD_JOBS_H
#define THREAD_JOBS_H


#include "glob_vars.h"
#include "queue.h"


void send_file_content(char* file, int client_socket);

void get_dir_content(char *path, int client_socket);

void count_files(char *path, int* total_files);


#endif //THREAD_JOBS_H