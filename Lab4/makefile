

logger.o: logger.c logger.h
	gcc logger.c -c -Wall -Wextra -Wpedantic -std=c11

all: lab4.c logger.o
	gcc lab4.c logger.o -o lab4comp -rdynamic -pthread -Wall -Wextra -Wpedantic -std=c11

clean:
	rm -f *.o lab4comp