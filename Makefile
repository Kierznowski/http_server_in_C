CC=gcc
CFLAGS=-Wall -pthread
OBJS=main.o http.o utils.o request.o

server: $(OBJS) #main file
	$(CC) $(CFLAGS) -o server $(OBJS)

clean:	#'make clean' command to remove compiled files
	rm -f *o server 