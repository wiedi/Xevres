CC=gcc
MYSQLI=-I/usr/include/mysql -I..
MYSQLL=-L/usr/lib/mysql -lmysqlclient
SRCS=testmod.c clearchan.c secretchannellist.c requestop.c opercommands.c trustcommands.c glinecommands.c spewcommands.c exceedingtg.c auth.c raw.c chan.c connect4.c
OBJS=$(SRCS:%.c=%.so)
OTHERFLAGS=-Wall -O3 -g -fPIC -shared -export-dynamic -Wl,-Bdynamic
Q=@

all: $(OBJS)

%.so: %.c
	@echo Compiling $<
	$(Q)$(CC) -o $@ $(MYSQLI) $(OTHERFLAGS) $<

clean:
	rm -f *.o *.bak *~ *.so
