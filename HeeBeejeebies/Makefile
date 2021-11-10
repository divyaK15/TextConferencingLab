CC=gcc
LDFLAGS=-pthread
all: server client 
server: server.o message.o message.h
deliver: client.o message.o message.h
clean: 
	rm -f *.o server client