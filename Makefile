all:
	gcc -o server -std=gnu99  server.c -lpthread
clean:
	rm server
