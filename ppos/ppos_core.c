#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"

void ppos_init () {
    setvbuf(stdout, 0, _IONBF, 0);
}

int task_init (task_t *task, void (*start_func)(void *), void *arg) {
    return 0;
}

int task_switch (task_t *task) {
    return 0;
}

void task_exit (int exit_code) {
    return;
}

int task_id () {
    return 0;
}