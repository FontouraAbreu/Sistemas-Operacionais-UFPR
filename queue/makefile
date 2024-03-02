CFLAGS = -Wall -g -O1 -std=c99
CC = gcc
# regras de ligação
all: testafila

testafila: queue.o testafila.o
	$(CC) -o testafila queue.o testafila.o -lm

# regras de compilação
queue.o: queue.c queue.h 
	$(CC) $(CFLAGS) -c queue.c -o queue.o
	
testafila.o: testafila.c
	$(CC) $(CFLAGS) -c testafila.c -o testafila.o




# regra para limpar os diretórios
run: testafila
	./testafila

clean:
	rm -f *.o testafila

purge: clean
	rm -f testafila