TARGET= search
CFLAGS = -c -pedantic -Wall --std=gnu99
OBJECTS = utils.o search.o

CC = gcc
LD = $(CC)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm $(TARGET) *.o

.PHONY: all clean

