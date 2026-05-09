FLAGS = -Wall -g
all: app

app: app.o client.o xdg.o shm.o
	gcc $(FLAGS) $^ -o app -lwayland-client

%.o: %.c
	gcc $(FLAGS) $< -c -o $@

clean:
	rm app *.o