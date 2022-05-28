all:
	gcc -g -o ./Server/server ./Server/server.c ./Server/queue.c -pthread
	gcc -o ./Client/client ./Client/client.c