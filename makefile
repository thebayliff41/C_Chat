CC := gcc
CFLAGS := -g
SOURCE := $(wildcard *.c)
OBJDIR := .objects
OBJECTS := $(addprefix $(OBJDIR)/,$(patsubst %.c, %.o, $(SOURCE)))
EXECS = server

server : $(OBJECTS)
	$(CC) $(CFLAGS) -o server $(OBJECTS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)

rs:
	./server

.PHONY : clean
clean:
	-rm -f $(OBJECTS) $(EXECS)
