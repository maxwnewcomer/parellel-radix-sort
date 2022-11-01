CC=gcc
CCFLAGS=-Wall -pthread -O0
LDFLAGS=
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
TARGET=test
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) 

%.o: %.c %.h
	$(CC) $(CCFLAGS) -c $<

%.o: %.c
	$(CC) $(CCFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)