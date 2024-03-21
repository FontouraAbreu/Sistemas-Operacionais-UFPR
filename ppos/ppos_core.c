#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"

//#define DEBUG
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t current_task, main_task;
ucontext_t old_context, current_context, main_context;
int task_count = 0;

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

    task->id = task_count++;
    task->status = 0;
    #ifdef DEBUG
        printf("[task_init] Tarefa %d criada\n", task->id);
    #endif
    return task->id;
}

int task_switch(task_t *task) {
    // saving current context
    if (getcontext(&old_context) == -1) {
        perror("[task_switch] Erro ao obter o contexto atual: ");
        return -1;
    }

    current_context = task->context;

    #ifdef DEBUG
        printf("[task_switch] Trocando contexto para %d\n", task->id);
    #endif

    if (swapcontext(&old_context, &current_context) == -1) {
        perror("[task_switch] Erro ao trocar de contexto: ");
        return -1;
    }

    return 0;
}

void task_exit(int exit_code) {
    #ifdef DEBUG
        printf("[task_exit] Tarefa %d encerrada\n", task_id());
    #endif
    if (exit_code != 0) {
        perror("[task_exit] Erro na finalização da tarefa: ");
    }
    swapcontext(&current_context, &main_context);
}

int task_id (){
    return current_task.id;
    return 0;
}