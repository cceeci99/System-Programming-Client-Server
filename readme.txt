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
  1) hostname -I to get the IP of server ->  ./dataServer -p <port> -q <queue_sz> -s <pool_sz> -b <block_sz>
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

1. ** Theoroume oti h fakelodomh tou Server vrisketai sto directory Server kai mono apo ekei mporei na zhthsei o Client, katalogous gia antigrafh.
To ektelesimo tou dataServer dhmioyrgeitai sto geniko project directory, enw to remoteClient mesa sto fakelo Client, opou kai tha grafontai ta apotelesmata.
Einai shmantiko to gegonos oti den prepei na vriskontai ston idio katalogo ta dyo ektelesima, wste auto na anaprosarmwsei to modelo Client, Server.

*****
Epeidh theloume omws to ektelesimo dataServer na paei na psaksei gia katalogous mesa sto Server kai MONO, prepei o kathe katalogos apo ton client na zhteitai me to relative path
dhladh: 

Server/foo/test1,   -> kanei copy to test1 
Server/foo/bar/test2, -> kanei copy to test2
Server, -> kanei copy olh thn ierarxia tou Server

* enw an zhththei sketo fakelos foo (o opoios den yparxei sto katalogo opou vrisketai to dataServer), den tha vrethei kathws to ektelesimo den vrisketai sto Server.
*****

Ola ta apotelesmata tha fainontai ston fakelo Client kai ekei tha sygkrinontai me auta toy Server apo ton user.

2. ** Gia aplothta, o kathe katalogos antigrafetai me to relative path mexri dhladh ton fakelo pou periexei to teliko arxeio. Dhladh an o Server
exei thn ekshsh ierarxia: 

-Server/foo/file2
-Server/foo/bar/file1

Kai o client tou zhthsei  -d Server/bar, o client telika tha parei ws apotelesma olh th fakelodomh dhladh: Server/foo/bar/file1, alla 
MONO o katalogos bar tha einai autos o opoios tha exei ginei copy me ola ta periexomena tou, enw gia paradeigma to file2 sto fakelo foo
den tha exei antigrafei.


3. Sxetika me thn leitourgia tou sygxronismou sto programma, einai profanes oti ean to 1o aithma einai arketa syntomo(mikro arithmo arxeiwn),
mporei na fainetai h seiriakh ektelesh twn aithmatwn mias kai tha prolavainei na oloklirwthei to kathe aithma prin erthei to epomen, milwntas panta
gia senaria treksimatos tou programmatos apo ton xrhsth.

******************
** To ektelesimo tou Client prepei na to treksete mesa apo to directory Client dhladh:
  cd Client
  ./remoteClient ...
******************