// GRR20206873 Vinicius Fontoura de Abreu
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "../queue/queue.h"

#define PRONTA 0
#define RODANDO 1
#define SUSPENSA 2
#define TERMINADA 3

//#define DEBUG
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t *current_task, main_task, dispatcher_task;
int task_count = 1;
queue_t *ready_queue, *suspended_queue;

task_t *scheduler() {
    // se a fila de prontas estiver vazia, retorna nulo
    if (ready_queue == NULL) {
        return NULL;
    }

    // troca a tarefa atual pela primeira tarefa da fila de prontas
    task_t *first_task_in_queue = (task_t *) ready_queue;
    // remove a tarefa da fila de prontas
    queue_remove(&ready_queue, (queue_t *) first_task_in_queue);

    // retorna a primeira tarefa da fila de prontas
    return first_task_in_queue;
}

void dispatcher_body(void *arg) {
    // retira o dispatcher da fila de prontas, para evitar que ele ative a si próprio
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);

    task_t *next_task;
    // enquanto houverem tarefas de usuário
    while (queue_size(ready_queue) > 0) {
        // escolhe a próxima tarefa a executar
        next_task = scheduler();

        // se houver uma próxima tarefa
        if (next_task) {
            // transfere o controle para a proxima tarefa
            next_task->status = RODANDO; // 1: rodando
            task_switch(next_task);

            switch (next_task->status) {
                case PRONTA: // 0
                    queue_append(&ready_queue, (queue_t *) next_task);
                    break;
                // se a tarefa foi suspensa, a coloca na fila de suspensas
                case SUSPENSA: // 2
                    queue_append(&suspended_queue, (queue_t *) next_task);
                    break;
                // se a tarefa foi encerrada, a remove da fila de prontas
                case TERMINADA: // 3
                    queue_remove((queue_t **) &ready_queue, (queue_t *) next_task);
                    free(next_task->context.uc_stack.ss_sp);
                    next_task->context.uc_stack.ss_sp = NULL;
                    next_task->context.uc_stack.ss_size = 0;
                    next_task = NULL;
                    break;
            }
        }
    }

    // encerra o dispatcher
    task_exit(0);
}


void ppos_init () {
    setvbuf(stdout, 0, _IONBF, 0);

    /* Inicializa o contexto da main */
    getcontext(&main_task.context);
    current_task = &main_task;
    main_task.id = 1;

    /* Inicializa as filas */
    ready_queue = NULL;
    suspended_queue = NULL;

    /* Inicializa a tarefa dispatcher */
    task_init(&dispatcher_task, (void *) dispatcher_body, NULL);
    queue_append(&ready_queue, (queue_t *) &dispatcher_task);

    #ifdef DEBUG
        printf("[ppos_init] PPos iniciado\n");
    #endif
}

int task_init(task_t *task, void (*start_func)(void *), void *arg) {
    /* Inicializa o contexto da task */
    getcontext(&task->context);

    /* Aloca a pilha do contexto */
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

    /* Cria o contexto da task */
    makecontext(&task->context, (void (*)(void)) start_func, 1, arg);

    /* Inicializa os campos da task */
    task->id = task_count++;
    task->status = PRONTA; // 0: pronta, 1: rodando, 2: suspensa
    #ifdef DEBUG
        printf("[task_init] Tarefa %d criada\n", task->id);
    #endif
    return task->id;
}

int task_switch(task_t *task) {
    /* Verifica se a tarefa é nula */
    if (task == NULL) {
        perror("[task_switch] Tarefa nula: ");
        return -1;
    }

    /* Salva a tarefa atual */
    task_t *old_task = current_task;
    current_task = task;

    #ifdef DEBUG
        printf("[task_switch] Trocando contexto para %d\n", task->id);
    #endif

    /* Troca o contexto */
    swapcontext(&old_task->context, &task->context);
    return 0;
}

void task_exit(int exit_code) {
    #ifdef DEBUG
        printf("[task_exit] Tarefa %d encerrada\n", task_id());
    #endif
    /* se o status de saída for diferente de 0, imprime erro */
    if (exit_code != 0) {
        perror("[task_exit] Erro na finalização da tarefa: ");
    }

    /* retorna para a tarefa principal */
    task_switch(&dispatcher_task);
}

int task_id (){
    /* se a tarefa for nula, imprime erro */
    if (!current_task){
        perror("[task_id] Tarefa nula: ");
        return 0;
    }
    return current_task->id;
}

void task_yield() {
    /* se a tarefa for nula, imprime erro */
    if (!current_task){
        perror("[task_yield] Tarefa nula: ");
        return;
    }


    current_task->status = PRONTA;
    queue_append(&ready_queue, (queue_t *) current_task);
    task_switch(&main_task);
}