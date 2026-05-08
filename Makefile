FLAGS = -Wall -g
all: client

client: client.o xdg.o shm.o
	gcc $(FLAGS) $^ -o client -lwayland-client

%.o: %.c
	gcc $(FLAGS) $< -c -o $@

clean:
	rm client *.o