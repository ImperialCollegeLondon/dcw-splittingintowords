#TOOLS	=	$(HOME)/c-tools
#LIBDIR  =       $(TOOLS)/lib/$(ARCH)
#INCDIR  =       $(TOOLS)/include
#CFLAGS  =       -I. -I$(INCDIR) -Wall -g
#LDLIBS  =       -L$(LIBDIR) -lset
CFLAGS  =       -Wall -g
LDLIBS  =
CC	=	gcc

all:	findlongest backtrack

findlongest:	findlongest.o set.o
	$(CC) -o findlongest $(LDLIBS) findlongest.o set.o

backtrack:	backtrack.o set.o
	$(CC) -o backtrack $(LDLIBS) backtrack.o set.o

clean:
	/bin/rm -f findlongest backtrack *.o core a.out
