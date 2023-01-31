CC := gcc
CFLAG := -g -Wall

all:server client
.PHONY:all

server:server.c
	$(CC) $^ -o $@ $(CFLAG)

client:client.c
	$(CC) $^ -o $@ $(CFLAG)

.PHONY:clean
clean:
	rm -f client server