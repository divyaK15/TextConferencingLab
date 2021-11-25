CC=gcc
LDFLAGS=-pthread
all: server sample_client 
server: server.o message.o message.h
sample_client: sample_client.o message.o message.h
clean: 
	rm -f *.o server sample_client 