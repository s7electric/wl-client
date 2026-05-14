FLAGS = -Wall -g
all: app libwlclient.a

libwlclient.a: client.o xdg.o shm.o
	ar rcs libwlclient.a $^

app: app.o client.o xdg.o shm.o
	cc $(FLAGS) $^ -o app -lwayland-client -lm

%.o: %.c
	cc $(FLAGS) $< -c -o $@

clean:
	rm app *.o *.a
