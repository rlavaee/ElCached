all: server

server: server.o
	g++ -g -std=c++11 $< -o $@ -levent -lpthread -llpsolve55 -ldl -lcolamd

.c.o:
	g++ -g -std=c++11 -c $< -o $@

clean:
	rm -rf *.o server
