CC=gcc
LDFLAGS=-pthread
all: server client 
server: server.o 
deliver: client.o 
clean: 
	rm -f *.o server client