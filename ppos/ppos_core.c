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

#define DISPATCHER_ID 1
#define MAIN_ID 0

#define DEFAULT_PRIO 0
#define HIGH_PRIO -20
#define LOW_PRIO 20
#define AGING_FACTOR -1

// #define DEBUG
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t *current_task, main_task, dispatcher_task;
int task_count = DISPATCHER_ID; // task_count começa em 1, pois o dispatcher é a primeira a ser criada
queue_t *ready_queue, *suspended_queue;

task_t *scheduler() {
    // se a fila de prontas estiver vazia, retorna nulo
    if (ready_queue == NULL) {
        #ifdef DEBUG
            printf("[scheduler] Fila de prontas vazia\n");
        #endif
        return NULL;
    }

    task_t *choosen_task = (task_t *) ready_queue;
    task_t *current_task = (task_t *) ready_queue;

    /* percorre a fila de prontas procurando pela tarefa com maior prioridade */
    do {
        task_t *next_task = (task_t *) current_task->next;

        // se a prioridade da tarefa atual for menor que a prioridade da próxima tarefa
        if (current_task->prio > next_task->prio) {
            choosen_task = next_task;
            
            if (current_task->prio > HIGH_PRIO)
                current_task->prio += AGING_FACTOR;
            else
                current_task->prio = HIGH_PRIO;
        }
        
        current_task = next_task;
    } while (current_task != (task_t *)ready_queue);    
    

    #ifdef DEBUG
        printf("[scheduler] Tarefa %d escolhida\n", choosen_task->id);
    #endif

    // retorna a primeira tarefa da fila de prontas
    return choosen_task;
}

void dispatcher_body(void *arg) {
    #ifdef DEBUG
        printf("[dispatcher_body] Iniciando dispatcher\n");
    #endif
    // retira o dispatcher da fila de prontas, para evitar que ele ative a si próprio
    queue_remove((queue_t **) &ready_queue, (queue_t *) &dispatcher_task);
    
    #ifdef DEBUG
        printf("[dispatcher_body] Dispatcher removido da fila de prontas\n");
    #endif

    task_t *next_task = NULL;
    // enquanto houverem tarefas de usuário
    while (queue_size(ready_queue) > 0) {
        // escolhe a próxima tarefa a executar
        #ifdef DEBUG
            printf("[dispatcher_body] Fila de prontas com %d tarefas\n", queue_size(ready_queue));
            printf("[dispatcher_body] Escolhendo próxima tarefa\n");
        #endif
        next_task = scheduler();

        // se houver uma próxima tarefa
        if (next_task) {

            // troca a tarefa atual pela primeira tarefa da fila de prontas
            task_t *first_task_in_queue = (task_t *) ready_queue;
            // remove a tarefa da fila de prontas
            queue_remove(&ready_queue, (queue_t *) first_task_in_queue);
            // transfere o controle para a proxima tarefa
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
                    #ifdef DEBUG
                        printf("[dispatcher_body] Desalocando pilha da tarefa %d\n", next_task->id);
                    #endif
                    free(next_task->context.uc_stack.ss_sp);
                    next_task->context.uc_stack.ss_sp = NULL;
                    next_task->context.uc_stack.ss_size = 0;
                    next_task = NULL;
                    break;
            }
        }
    }

    #ifdef DEBUG
        printf("[dispatcher_body] Todas as tarefas encerradas, encerrando dispatcher\n");
    #endif

    // encerra o dispatcher
    task_exit(0);
}


void ppos_init () {
    setvbuf(stdout, 0, _IONBF, 0);

    /* Inicializa o contexto da main */
    getcontext(&main_task.context);
    current_task = &main_task;
    main_task.id = MAIN_ID;

    /* Inicializa as filas */
    ready_queue = NULL;
    suspended_queue = NULL;

    /* Inicializa a tarefa dispatcher */
    #ifdef DEBUG
        printf("[dispatcher_body] Tarefa dispatcher criada\n");
    #endif

    task_init(&dispatcher_task, (void *) dispatcher_body, NULL);

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
        perror("WARNING [task_init] Erro na criação da pilha: ");
        return -1;
    }

    /* Cria o contexto da task */
    makecontext(&task->context, (void (*)(void)) start_func, 1, arg);

    /* Inicializa os campos da task */
    task->id = task_count++;
    task->status = PRONTA; // 0: pronta, 1: rodando, 2: suspensa
    task->prio = DEFAULT_PRIO;

    /* Adiciona a task na fila de prontas */
    queue_append(&ready_queue, (queue_t *) task);

    #ifdef DEBUG
        printf("[task_init] Tarefa %d criada e adicionada na fila de prontas\n", task->id);
        printf("[task_init] Fila de prontas com %d tarefas\n", queue_size(ready_queue));
    #endif

    return task->id;
}

int task_switch(task_t *task) {
    /* Verifica se a tarefa é nula */
    if (task == NULL) {
        perror("WARNING [task_switch] Tarefa nula: ");
        return -1;
    }

    /* Salva a tarefa atual */
    task_t *old_task = current_task;
    current_task = task;

    #ifdef DEBUG
        int id = task_id();
        if (id == DISPATCHER_ID)
            printf("[task_switch] Trocando contexto para o dispatcher\n");
        else
            printf("[task_switch] Trocando contexto para %d\n", id);
    #endif

    task->status = RODANDO; // 1: rodando
    /* Troca o contexto */
    swapcontext(&old_task->context, &task->context);
    return 0;
}

void task_exit(int exit_code) {
    /* se o status de saída for diferente de 0, imprime erro */
    if (exit_code != 0) {
        perror("WARNING [task_exit] Erro na finalização da tarefa: ");
    }


    /* se a tarefa que está encerrando é o dispatcher */
    switch (task_id()) {
        case MAIN_ID: // tarefa Main
            #ifdef DEBUG
                printf("[task_exit] Tarefa Main encerrada\n");
            #endif
            current_task->status = TERMINADA;
            task_switch(&dispatcher_task);
            break;
        case DISPATCHER_ID: // tarefa Dispatcher
            #ifdef DEBUG
                printf("[task_exit] Tarefa Dispatcher encerrada\n");
            #endif
            exit(exit_code);
            break;
        default:
            /* retorna para a tarefa principal */
            #ifdef DEBUG
                printf("[task_exit] Tarefa %d encerrada\n", task_id());
            #endif
            task_count--;
            current_task->status = TERMINADA;
            task_switch(&dispatcher_task);
            break;
    }
}

int task_id (){
    /* se a tarefa for nula, imprime erro */
    if (!current_task){
        perror("WARNING [task_id] Tarefa nula: ");
        return -1;
    }
    return current_task->id;
}

void task_yield() {
    /* se a tarefa for nula, imprime erro */
    if (!current_task){
        perror("WARNING [task_yield] Tarefa nula: ");
        return;
    }

    current_task->status = PRONTA;
    task_switch(&dispatcher_task);
}

void task_setprio(task_t *task, int prio) {
    /* se a tarefa for nula, imprime erro */
    if (!task){
        perror("WARNING [task_setprio] Tarefa nula: ");
        return;
    }

    task->prio = prio;
}

int task_getprio(task_t *task) {
    /* se a tarefa for nula, imprime erro */
    if (!task){
        perror("WARNING [task_getprio] Tarefa nula: ");
        return -1;
    }

    return task->prio;
}