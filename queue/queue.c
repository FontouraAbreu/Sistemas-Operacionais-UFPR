#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int queue_size (queue_t *queue) {
    queue_t *current_node = queue;
    int count = 0;
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
    return 0;
}

int queue_remove (queue_t **queue, queue_t *elem) {
    return 0;
}
