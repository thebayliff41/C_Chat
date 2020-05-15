CC := gcc
CFLAGS := -g -Wall -Wextra -pedantic -Wno-unused-parameter
SOURCE := $(wildcard *.c)
DEPS := $(wildcard *.h)
OBJDIR := .objects
OBJECTS := $(addprefix $(OBJDIR)/,$(patsubst %.c, %.o, $(SOURCE)))
EXECS := server server_shutdown client start_server

NCURSES = -lncurses

_CLIENT_OBJ := client.o
CLIENT_OBJ := $(patsubst %,$(OBJDIR)/%,$(_CLIENT_OBJ))

_SERVER_OBJ := server.o list.o
SERVER_OBJ := $(patsubst %,$(OBJDIR)/%,$(_SERVER_OBJ))

_SHUTDOWN_OBJ := server_shutdown.o
_SHUTUDOWN_OBJ := $(patsubst %,$(OBJDIR)/%,$(_SHUTDOWN_OBJ))

main: $(EXECS)

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

server_shutdown: $(SHUTDOWN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(NCURSES)

start_server: start_server.c
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)

rs:
	./start_server

ks:
	./server_shutdown

rc:
	./client Bailey

.PHONY : clean
clean:
	-rm -f $(OBJECTS) $(EXECS)
