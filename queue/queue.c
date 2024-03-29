// GRR20206873 Vinicius Fontoura de Abreu
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int queue_size (queue_t *queue) {
    queue_t *current_node = queue;
    int count = 0;

    // se a fila não existir, então o tamanho é 0
    if (queue == NULL) {
        return 0;
    }

    // se a fila existir, então o tamanho é a quantidade de elementos na fila
    do {
        count++;
        current_node = current_node->next;
    } while (current_node != queue);

    return count;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    queue_t *current_node = queue;
    if (queue == NULL) {
        printf("%s: []\n", name);
        return;
    }

    printf("%s: [", name);
    do {
        print_elem((void*) current_node);
        if (current_node->next != queue) {
            printf(" ");
        }
        current_node = current_node->next;
    } while (current_node != queue);
    printf("]\n");

    return;
}

int queue_append (queue_t **queue, queue_t *elem) {
    // - a fila deve existir
    if (queue == NULL) {
        perror("WARNING [queue_append] fila não existe\n");
        return -1;
    }

    // - o elemento deve existir
    if (elem == NULL) {
        perror("WARNING [queue_append] elemento não existe\n");
        return -2;
    }

    // - o elemento nao deve estar em outra fila
    // se o elemento já está em outra fila, então elem->next e elem->prev não podem ser NULL
    if (elem->next != NULL || elem->prev != NULL) {
        perror("WARNING [queue_append] elemento já está em outra fila\n");
        return -3;
    }

    // caso nenhuma das condições acima seja verdadeira, então o elemento pode ser adicionado à fila

    // se a fila estiver vazia
    if (*queue == NULL) {
        *queue = elem;
        elem->next = elem;
        elem->prev = elem;
    } else {
        // se a fila não estiver vazia
        queue_t *last = (*queue)->prev;
        last->next = elem;
        elem->prev = last;
        elem->next = *queue;
        (*queue)->prev = elem;
    }
    return 0;
}

int queue_remove (queue_t **queue, queue_t *elem) {
    // - a fila deve existir
    if (queue == NULL) {
        perror("WARNING [queue_remove] fila não existe\n");
        return -1;
    }

    // - a fila nao deve estar vazia
    if (queue_size(*queue) == 0) {
        perror("WARNING [queue_remove] fila está vazia\n");
        return -2;
    }

    // - o elemento deve existir
    if (elem == NULL) {
        perror("WARNING [queue_remove] elemento não existe\n");
        return -3;
    }

    // - o elemento deve pertencer a fila indicada
    queue_t *current_node = *queue;
    do {
        if (current_node == elem) {
            break;
        }
        current_node = current_node->next;

    } while (current_node != *queue);

    if (current_node != elem) {
        perror("WARNING [queue_remove] elemento não pertence à fila indicada\n");
        return -4;
    }

    // caso nenhuma das condições acima seja verdadeira, então o elemento pode ser removido da fila
    // se a fila tiver apenas um elemento
    if (queue_size(*queue) == 1) {
        (*queue)->next = NULL;
        (*queue)->prev = NULL;
        *queue = NULL;
    } else { // se a fila tiver mais de um elemento
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
        // se o elemento a ser removido é o primeiro da fila
        if (*queue == elem) {
            queue_t *aux = elem->prev;
            *queue = elem->next;
            (*queue)->prev = aux;
        }
    }
    // reseta os ponteiros do elemento removido
    elem->next = NULL;
    elem->prev = NULL;

    return 0;
}
