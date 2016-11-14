CC=gcc
CFLAGS=-c -g -Wall 
LDFLAGS= -pthread
SOURCES=usbserial.c usbserial_linux.c rbuff.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=usbserial

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
	
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	
install:
	cp -p $(EXECUTABLE) ~/bin
