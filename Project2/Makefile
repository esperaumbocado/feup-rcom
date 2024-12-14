CC = gcc
CFLAGS = -Wall -g
OBJS = download.o parser.o socket.o transfer.o

all: download

download: $(OBJS)
	$(CC) $(CFLAGS) -o download $(OBJS)

download.o: download.c parser.h socket.h transfer.h constants.h
	$(CC) $(CFLAGS) -c download.c

parser.o: parser.c parser.h constants.h
	$(CC) $(CFLAGS) -c parser.c

socket.o: socket.c socket.h constants.h
	$(CC) $(CFLAGS) -c socket.c

transfer.o: transfer.c transfer.h constants.h
	$(CC) $(CFLAGS) -c transfer.c

clean:
	rm -f *.o download
