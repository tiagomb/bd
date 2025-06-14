CC = gcc
CFLAGS = -Wall -g -O3

TARGET = escalona
SOURCES = escalona.c schedule.c serializability.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = schedule.h serializability.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

removeObjects:
	rm -f $(OBJECTS)

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean