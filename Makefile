FLAGS = -Wall -g
all: app

app: app.o client.o xdg.o shm.o
	cc $(FLAGS) $^ -o app -lwayland-client -lm

%.o: %.c
	cc $(FLAGS) $< -c -o $@

clean:
	rm app *.o