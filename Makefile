all:
	gcc -o ./Server/server ./Server/server.c ./Server/queue.c
	gcc -o ./Client/client ./Client/client.c

clean:
	rm -rf ./Client/bar