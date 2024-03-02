#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int queue_size (queue_t *queue) {
    queue_t *current_node = queue;
    int count = 0;
    if (queue == NULL) {
        return 0;
    }
    do {
        count++;
        current_node = current_node->next;
    } while (current_node != queue);

    return count;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    queue_t *current_node = queue;

    do {
        print_elem((void*) current_node);
    } while (current_node != queue);

    return;
}

int queue_append (queue_t **queue, queue_t *elem) {
    // - a fila deve existir
    if (queue == NULL) {
        return -1;
    }

    // - o elemento deve existir
    if (elem == NULL) {
        return -2;
    }

    // - o elemento nao deve estar em outra fila
    // se o elemento já está em outra fila, então elem->next e elem->prev não podem ser NULL
    if (elem->next != NULL || elem->prev != NULL) {
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
    return 0;
}
