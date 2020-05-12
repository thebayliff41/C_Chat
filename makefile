CC := gcc
CFLAGS := -g
SOURCE := $(wildcard *.c)
DEPS := $(wildcard *.h)
OBJDIR := .objects
OBJECTS := $(addprefix $(OBJDIR)/,$(patsubst %.c, %.o, $(SOURCE)))
EXECS := server server_shutdown

_SERVER_OBJ := server.o list.o
SERVER_OBJ := $(patsubst %,$(OBJDIR)/%,$(_SERVER_OBJ))

_SHUTDOWN_OBJ := server_shutdown.o
_SHUTUDOWN_OBJ := $(patsubst %,$(OBJDIR)/%,$(_SHUTDOWN_OBJ))

main: $(EXECS)

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

server_shutdown: $(_SHUTDOWN_OBJ)
	echo $(_SHUTDOWN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)

rs:
	./server

shutdown:
	./server_shutdown

.PHONY : clean
clean:
	-rm -f $(OBJECTS) $(EXECS)
