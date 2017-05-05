#TOOLS	=	$(HOME)/c-tools
#LIBDIR  =       $(TOOLS)/lib/$(ARCH)
#INCDIR  =       $(TOOLS)/include
#CFLAGS  =       -I. -I$(INCDIR) -Wall -g
#LDLIBS  =       -L$(LIBDIR) -lset
CFLAGS  =       -Wall -g
LDLIBS  =
CC	=	gcc

all:	findlongest

findlongest:	findlongest.o set.o
	$(CC) -o findlongest $(LDLIBS) findlongest.o set.o

clean:
	/bin/rm -f findlongest *.o core a.out
