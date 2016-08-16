
CC	=	g++
# use the following cflags for old versions of g++ with sets.old.H
#OLD_CFLAGS	=	-g -Wall -pedantic -fguiding-decls
# use the following for new versions of g++/egcs with sets.H
CFLAGS	=-fPIC -pedantic -std=c++11 -O3 -Wall
LINK	=	g++
LFLAGS	=	${CFLAGS} -llpsolve55 -lcolamd -lm -ldl
OBJECTS = test.o  LP.o

all: test.exe

.PHONY: test.exe clean 

clean:
	rm -f test *.o

%.o : %.cc
	$(CC) $(CFLAGS) -c $< -o $@

test.exe : $(OBJECTS)
	${LINK} $(OBJECTS) ${LFLAGS} -o $@
