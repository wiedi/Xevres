This directory contains Doug Leas malloc() routines.
Further info on them can be found at
http://gee.cs.oswego.edu/dl/html/malloc.html
If you are on BSD, it is _HIGHLY_ recommended to use these routines instead
of BSDs default malloc() stuff when compiling O. I'm talking about as much
as 40% less memory usage here!
To use this routines, compile them with "make", and then modify the
Makefile in O's main directory as follows:
$(CC) -o Operservice -ldl $(OTHERFLAGS) $(MYSQLI) $(OBJS) $(MYSQLL)
must instead read:
$(CC) -o Operservice -ldl $(OTHERFLAGS) $(MYSQLI) dlmalloc/malloc.o $(OBJS) $(MYSQLL)
