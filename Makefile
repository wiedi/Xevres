CC=gcc
MAKE=make
MYSQLI=-I/usr/include/mysql
MYSQLL=-L/usr/lib/mysql -lmysqlclient
SRCS=Operservice.c subnetlist.c subnettrust.c fakeuser.c realnamegline.c stringtools.c general.c serverhandlers.c usercommands.c splitdb.c arrays.c dynamic.c trusts.c md5.c chandb.c chancheck.c
OBJS=$(SRCS:.c=.o)
OTHERFLAGS=-Wall -O3 -g -fPIC -export-dynamic

all: $(OBJS) globals.h config.h stringtools.h
	$(CC) -o Operservice -ldl $(OTHERFLAGS) $(MYSQLI) $(OBJS) $(MYSQLL)
	cd modules ; $(MAKE)

.c.o:
	$(CC) $(MYSQLI) $(OTHERFLAGS) -c $<

clean:
	rm -f Operservice *.o *.bak *~
	cd modules ; $(MAKE) clean
