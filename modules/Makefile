CC=gcc
MYSQLI=-I/usr/include/mysql -I..
MYSQLL=-L/usr/lib/mysql -lmysqlclient
SRCS=testmod.c clearchan.c secretchannellist.c requestop.c opercommands.c trustcommands.c glinecommands.c spewcommands.c exceedingtg.c
OBJS=$(SRCS:%.c=%.so)
OTHERFLAGS=-Wall -O3 -g -fPIC -shared -export-dynamic -Wl,-Bdynamic

all: $(OBJS)

%.so: %.c
	$(CC) -o $@ $(MYSQLI) $(OTHERFLAGS) $<

clean:
	rm -f *.o *.bak *~ *.so
