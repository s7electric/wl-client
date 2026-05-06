all: client

client: client.c 
	gcc -Wall -g client.c -o client -lwayland-client

clean:
	rm client