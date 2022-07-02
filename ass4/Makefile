CFLAGS = -pedantic -Wall --std=gnu99 -D_GNU_SOURCE 
LDFLAGS = -lpthread
COMMON_OBJECTS = utils.o common.o netutils.o
CLIENT_OBJECTS = client.o
CLIENT_TARGET = client
SERVER_TARGET = server
SERVER_OBJECTS = server.o

all: $(CLIENTBOT_TARGET) $(SERVER_TARGET) $(CLIENT_TARGET)

CC = gcc
LD = $(CC)

$(CLIENT_TARGET): $(CLIENT_OBJECTS) $(COMMON_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

$(SERVER_TARGET): $(SERVER_OBJECTS) $(COMMON_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^
clean:
	rm $(CLIENT_TARGET) $(SERVER_TARGET) *.o 

.PHONY: all clean

