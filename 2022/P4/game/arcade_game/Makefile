all: server.c client.c
	gcc -o server server.c -lpthread
	gcc client.c -lpthread -lncurses -o client
