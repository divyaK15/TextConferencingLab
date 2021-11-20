CC=gcc
LDFLAGS=-pthread
all: server client sample_client new_server
server: server.o message.o message.h
client: client.o message.o message.h
sample_client: sample_client.o message.h
new_server: new_server.o message.h
clean: 
	rm -f *.o server client sample_client new_server