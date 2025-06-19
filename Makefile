CC=gcc
CFLAGS=-Wall -pthread -I./src
SRCDIR = src
BUILDDIR = build

SRCFILES = main.c http.c utils.c request.c router.c http_errors.c server.c
OBJFILES = $(SRCFILES:.c=.o)

SRCS = $(addprefix $(SRCDIR)/, $(SRCFILES))
OBJS = $(addprefix $(BUILDDIR)/, $(OBJFILES))

server: $(OBJS)
	$(CC) $(CFLAGS) -o server $(OBJS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:	#'make clean' remove compiled files
	rm -rf  $(BUILDDIR) server