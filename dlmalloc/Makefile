CC=gcc
SRCS=malloc.c
OBJS=$(SRCS:%.c=%.o)
OTHERFLAGS=-Wall -O3 -g -fPIC -export-dynamic

all: $(OBJS)

%.o: %.c
	$(CC) -o $@ $(MYSQLI) $(OTHERFLAGS) $<

clean:
	rm -f *.o *.bak *~ *.so
