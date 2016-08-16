
CC	=	g++
# use the following cflags for old versions of g++ with sets.old.H
#OLD_CFLAGS	=	-g -Wall -pedantic -fguiding-decls
# use the following for new versions of g++/egcs with sets.H
CFLAGS	=-fPIC -pedantic -std=c++11 -O3 -Wall
LINK	=	g++
LFLAGS	=	${CFLAGS} -ldl -lpthread -lboost_system -llpsolve55 -lcolamd -levent -lrt
OBJECTS = test.o RedBlackTree.o LP.o

all: librd_trace.so

.PHONY: test clean 

test: test.exe
	./test.exe

clean:
	rm -f test.exe *.o *.so

%.o : %.cc
	$(CC) $(CFLAGS) -c $< -o $@

test.exe : $(OBJECTS)
	${LINK} ${LFLAGS} $(OBJECTS) -o test.exe

librd_trace.so: rd_trace.o RedBlackTree.o LP.o
	${LINK} ${LFLAGS} -shared -O3 -fPIC -o $@ $^
