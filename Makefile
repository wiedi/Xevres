CC=gcc
MAKE=make
MYSQLI=-I/usr/include/mysql
MYSQLL=-L/usr/lib/mysql -lmysqlclient
SRCS=xevres.c subnetlist.c subnettrust.c fakeuser.c realnamegline.c stringtools.c general.c serverhandlers.c usercommands.c splitdb.c arrays.c dynamic.c trusts.c md5.c chandb.c chancheck.c
OBJS=$(SRCS:.c=.o)
OTHERFLAGS=-Wall -O3 -g -fPIC -export-dynamic
Q=@
all: $(OBJS) globals.h config.h stringtools.h
	@echo Compiling Xevres
	$(Q)$(CC) -o Xevres -ldl $(OTHERFLAGS) $(MYSQLI) $(OBJS) $(MYSQLL)
	cd modules ; $(MAKE)

.c.o:
	@echo Compiling $<
	$(Q)$(CC) $(MYSQLI) $(OTHERFLAGS) -c $<
clean:
	rm -f Xevres *.o *.bak *~
	cd modules ; $(MAKE) clean
