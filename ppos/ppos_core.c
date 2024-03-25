#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"

//#define DEBUG
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t *current_task, main_task;
int task_count = 1;

void ppos_init () {
    setvbuf(stdout, 0, _IONBF, 0);

    getcontext(&main_task.context);
    current_task = &main_task;
    main_task.id = 1;
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

    task->id = task_count++;
    task->status = 0;
    #ifdef DEBUG
        printf("[task_init] Tarefa %d criada\n", task->id);
    #endif
    return task->id;
}

int task_switch(task_t *task) {

    if (task == NULL) {
        perror("[task_switch] Tarefa nula: ");
        return -1;
    }

    task_t *old_task = current_task;
    current_task = task;

    #ifdef DEBUG
        printf("[task_switch] Trocando contexto para %d\n", task->id);
    #endif

    swapcontext(&old_task->context, &task->context);
    return 0;
}

void task_exit(int exit_code) {
    #ifdef DEBUG
        printf("[task_exit] Tarefa %d encerrada\n", task_id());
    #endif
    if (exit_code != 0) {
        perror("[task_exit] Erro na finalização da tarefa: ");
    }

    if (&main_task == NULL) {
        perror("[task_exit] Tarefa nula: ");
        return;
    }

    task_switch(&main_task);
}

int task_id (){
    if (!current_task){
        perror("[task_id] Tarefa nula: ");
        return 0;
    }
    return current_task->id;
}