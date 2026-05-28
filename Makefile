FLAGS = -Wall -g

default: libwlclient.a

all: example libwlclient.a

libwlclient.a: wlclient.o xdg.o shm.o
	ar rcs libwlclient.a $^

example: example/example.c libwlclient.a
	cc $(FLAGS) $^ -o example/example -lwayland-client -lm

%.o: %.c %.h
	cc $(FLAGS) $< -c -o $@

clean:
	rm *.o *.a
	rm example/example
