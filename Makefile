all:
	gcc -o ./dataServer ./Server/src/dataServer.c ./Server/src/queue.c ./Server/src/thread_jobs.c -pthread
	gcc -o ./Client/remoteClient ./Client/remoteClient.c

clean:
	rm ./dataServer ./Client/remoteClient