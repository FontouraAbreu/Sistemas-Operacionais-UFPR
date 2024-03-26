# GRR20206873 Vinicius Fontoura de Abreu
CFLAGS = -Wall -g -std=c99
CC = gcc
TEST_FILE = pingpong-dispatcher.c
# regras de ligação
all: ppos

ppos: queue.o ppos_core.o test.o
	$(CC) -o ppos_core queue.o ppos_core.o test.o -lm

# regras de compilação
queue.o: ../queue/queue.c ../queue/queue.h 
	$(CC) $(CFLAGS) -c ../queue/queue.c -o queue.o
	
ppos_core.o: ppos_core.c
	$(CC) $(CFLAGS) -c ppos_core.c -o ppos_core.o

test.o: $(TEST_FILE) queue.o ppos_core.o
	$(CC) $(CFLAGS) -c $(TEST_FILE) -o test.o -lm



# regra para limpar os diretórios
run: ppos
	./ppos

clean:
	rm -f *.o ppos

purge: clean
	rm -f ppos