# GRR20206873 Vinicius Fontoura de Abreu
CFLAGS = -Wall -g -std=gnu99
LFLAGS = -lpthread -lm
CC = gcc
TEST_FILE = pingpong-mqueue.c
# regras de ligação
all: ppos

ppos: queue.o ppos_core.o test.o
	$(CC) -o ppos_core queue.o ppos_core.o test.o $(LFLAGS)

# regras de compilação
queue.o: queue.c queue.h 
	$(CC) $(CFLAGS) -c queue.c -o queue.o
	
ppos_core.o: ppos_core.c
	$(CC) $(CFLAGS) -c ppos_core.c -o ppos_core.o

test.o: $(TEST_FILE) queue.o ppos_core.o
	$(CC) $(CFLAGS) -c $(TEST_FILE) -o test.o $(LFLAGS)



# regra para limpar os diretórios
run: ppos
	./ppos_core

clean:
	rm -f *.o ppos_core

purge: clean
	rm -f ppos_core