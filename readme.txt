        |--------------   README:  System Programming, Project2, Semester: 6th, 2022  ------------------|
        |                                                                                               |
        |        Name:  TSVETOMIR IVANOV                                                                |
        |        sdi: 1115201900066                                                                     |
        |                                                                                               |
        |-----------------------------------------------------------------------------------------------|


-       -       -       -       -       -       -       -       -      -       -       -       -       -      -


Project's Contents
=============================================================

1. Makefile
2. readme.txt

Running Program:
---------------
- make
- in splitted terminal  
  1) cd Server -> hostname -I to get the IP of server ->  ./dataServer -p <port> -q <queue_sz> -s <pool_sz> -b <block_sz>
  2) cd Client -> ./remoteClient -p <port> -i <server's IP> -d <dir>

to see the functionality with many clients, a script would help to spawn more than one clients.

Client Directory:
-----------------

remoteClient.c -> source file for implementing the Client program. 
remoteClient -> executable which is made with Makefile to run

All directories will be stored in Client's directory. If two clients ask for same directory, only one of them will be made, and there will be not any duplicates.

Implementation of Client:
------------------------

Client connects to server via socket, he sends the directory's name and after that he reads the number of files that will be copied. In that way he can know
how many files he will accept and does the reading of each filename one by one with for loop. For each filename that he accepts, he reads also the size of the file
in bytes and start copying contents block by block (with block_sz which is passed first through socket). Finally he makes the directory(if it doesn't exists) and 
creates the file (deletes it if it exists).


Server Directory:
------------------

dataServer.c -> source file for implemeting Server program
dataServer -> executable which is made with Makefile to run

glob_vars.h -> a header file in which all global variables for the server programm are declared.
  In those variables there are, the Queue, all of the Mutexes and Condition Variables for the synchronizing of threads, also some defines and sizes that are used
  in thread's functions.

queue.c, queue.h -> A utility for the Queue struct that is used by threads.
  The queue contains a two items,  <Filename, Socket_fd>. This is the data that worker_thread needs to start sending the data of the file to the client
  specified with the socket_fd.

thread_jobs.c, thread_jobs.h -> A source and header file for the functions that are used by the two kind of threads.
  The functions are:

  1. get_dir_content(dir, socket): Used by the communication_thread, to scan the directory and push all the files in the queue.
  2. count_files(dir): Used by the communication_thread to scan the directory at first and count the number of files for copy and send it to the client
  
  3. send_file_content(file, socket): Used by the worker_thread, to start sending the data of the file to the client.


Implementation of Server:
-------------------------

At first server initializes all needed variables from glob_vars.h that are used in the threads (queue, mutexes etc).
After that he opens the socket and start waiting in a infinty loop, for connections from clients. For each client new mutex is created based on it's socket_fd
(client_mutex struct) and is initialized. In that way we can exclude that 2 or more worker_threads will write to the same socket. Lastly he spawns the communication_thread.

The two threads are implemented with the functions:

1. communication_thread_t():
  - Reads the directory's name
  - count the files to copy and send them to client
  - scan the directory and for each file push it to the queue
  - for the access of the queue, locks the queue_mutex, and waits on cond queue_full_cond while the queue is full.
  - signals the queue_empty_cond when new file is pushed to the queue

2. worker_thread_t():
  - wait continuously in infinty loop for new job
  - for the access to the queue locks the queue_mutex, -> pop() from the queue
  - waits on queue_empty_cond while the queue is empty
  - signals the queue_full_cond when one file is poped from the queue
  - to write on specified client (based on the socket), finds the corresponding mutex and locks it
  - calls send_file_content() to write the data block by block to the socket
  - unlocks the client_mutex

Notes: (Paradoxes gia thn ergasia)
-----------------------------------

1. For implemeting more simple the copying of folders, each folder is copied with the relative path and all subdirectories are created.
For example if client wants to copy test/fold1/fold2 then the result in Client's hierarchy will be:  test/fold1/fold2 and not only fold2.

2. Running the program, with some script that spawns many clients, if the directories are small with small number of files, it seems as it runs "serially"
meaning each client's request completes before the new one comes. If first client has a lot of files to copy and takes some time then the second  client's request
will be fulffiled first.