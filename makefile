CC = gcc
CFLAGS = -pthread

all: main.o server.o client.o
	$(CC) $(CFLAGS) main.o server.o client.o -o iperf -lm

main.o: main.c
	$(CC) -c main.c -lm

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -lm 

client.o: client.c
	$(CC) $(CFLAGS) -c client.c -lm

clean:
	rm -f *.o iperf output.json
