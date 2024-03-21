#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"

//#define DEBUG
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */


void ppos_init () {
    setvbuf(stdout, 0, _IONBF, 0);
}

int task_init(task_t *task, void (*start_func)(void *), void *arg) {
    getcontext(&task->context);

    char *stack = malloc(STACKSIZE);
    if (stack) {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    } else {
        perror("[task_init] Erro na criação da pilha: ");
        return -1;
    }

    makecontext(&task->context, (void (*)(void)) start_func, 1, arg);

    task->id = rand();
    task->status = 0;
    #ifdef DEBUG
        printf("[task_init] Tarefa %d criada\n", task->id);
    #endif
    return task->id;
}

int task_switch(task_t *task) {
    ucontext_t current_context;
    getcontext(&current_context);

    swapcontext(&current_context, &task->context);

    return 0;
}

void task_exit(int exit_code) {
    return;
}

int task_id (){
    
    return 0;
}