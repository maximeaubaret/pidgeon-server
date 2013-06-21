all:
	gcc -g src/*.c -std=c99 $(shell pkg-config --cflags --libs glib-2.0) -lpthread -o pidgeon-server
