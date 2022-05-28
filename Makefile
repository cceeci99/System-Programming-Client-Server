all:
	gcc -o ./Server/dataServer ./Server/dataServer.c ./Server/queue.c ./Server/thread_jobs.c -pthread
	gcc -o ./Client/remoteClient ./Client/remoteClient.c

clean:
	rm ./Server/dataServer ./Client/remoteClient