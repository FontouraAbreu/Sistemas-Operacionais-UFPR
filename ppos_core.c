// GRR20206873 Vinicius Fontoura de Abreu
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>


#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

/* STATUS DAS TAREFAS */
#define PRONTA 0
#define RODANDO 1
#define SUSPENSA 2
#define TERMINADA 3
#define DORMINDO 4

/* IDS DA MAIN E DISPATCHER */
#define DISPATCHER_ID 1
#define MAIN_ID 0

#define SYSTEM_TASK 1
#define USER_TASK 0

/* PRIORIDADES */
#define DEFAULT_PRIO 0
#define HIGH_PRIO -20
#define LOW_PRIO 20
#define AGING_FACTOR -1

/* DEFINES DO CLOCK */
#define CLOCK_INITIAL_VALUE_S 0
#define CLOCK_INITIAL_VALUE_US 1000 // 1ms
#define CLOCK_INTERVAL_VALUE_S 0
#define CLOCK_INTERVAL_VALUE_US 1000 // 1ms
#define QUANTUM 20


// #define DEBUG
#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t *current_task, main_task, dispatcher_task; // tarefas atual, principal e dispatcher
int task_count = 1; // contador de tarefas deve começar em 1
queue_t *ready_queue, *sleeping_queue; // filas de prontas e dormindo

unsigned int clock; // clock do sistema


struct sigaction action;
struct itimerval timer;

/* TIME HANDLER */
/* -------------------------------------------------------------------------- */

void tick_handler(int signum) {
    clock++;

    /* se a tarefa atual for o dispatcher, retorna */
    if (current_task->type == SYSTEM_TASK || --current_task->quantum > 0) 
        return;

    /* decrementa o quantum da tarefa atual */
    task_yield();
}

unsigned int systime() {
    return clock;
}

/* TIME HANDLER */
/* -------------------------------------------------------------------------- */


/* CORE */
/* -------------------------------------------------------------------------- */

task_t *scheduler() {
    // se a fila de prontas estiver vazia, retorna nulo
    if (ready_queue == NULL) {
        #ifdef DEBUG
            printf("[scheduler] Fila de prontas vazia\n");
        #endif
        return NULL;
    }

    task_t *choosen_task = (task_t *) ready_queue;
    task_t *loop_current_task = (task_t *) ready_queue;

    /* percorre a fila de prontas procurando pela tarefa com maior prioridade */
    do {
        task_t *next_task = (task_t *) loop_current_task->next;
        int next_task_total_prio = task_getprio(next_task) + next_task->prio_d;
        int choosen_task_total_prio = task_getprio(choosen_task) + choosen_task->prio_d;
        // se a prioridade da proxima tarefa for menor que a prioridade da tarefa atual
        if (next_task_total_prio < choosen_task_total_prio) {
            #ifdef DEBUG
                printf("[scheduler] Tarefa %d tem prioridade %d maior que tarefa %d com prioridade %d\n", next_task->id, next_task_total_prio, choosen_task->id, choosen_task_total_prio);
            #endif
            choosen_task = next_task;
        }
        
        // avança para a próxima tarefa
        loop_current_task = next_task;
    } while (loop_current_task != (task_t *)ready_queue);    

    // envelhece as tarefas
    loop_current_task = (task_t *) ready_queue;
    do {
        // se a tarefa atual for a tarefa escolhida
        if (loop_current_task == choosen_task) {
            #ifdef DEBUG
                printf("[scheduler] Ignorando a tarefa escolhida (%d) durante o envelhecimento\n", choosen_task->id);
            #endif
            loop_current_task = (task_t *) loop_current_task->next;
            continue;
        }

        // se a prioridade não atingiu o limite superior
        int total_prio = task_getprio(loop_current_task) + loop_current_task->prio_d;
        if (total_prio > HIGH_PRIO)
            loop_current_task->prio_d += AGING_FACTOR;
        
        #ifdef DEBUG
            printf("[scheduler] Envelhecendo tarefa %d para prioridade %d\n", loop_current_task->id, total_prio);
        #endif

        // avança para a próxima tarefa
        loop_current_task = (task_t *) loop_current_task->next;
    // enquanto não chegar ao final da fila de prontas
    } while (loop_current_task != (task_t *) ready_queue);
    

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
    /* variáveis para calcular o tempo de execução das tarefas */
    unsigned int current_task_start_time = 0;
    unsigned int current_task_end_time = 0;

    // enquanto houverem tarefas de usuário
    while (queue_size(ready_queue) > 0 || queue_size(sleeping_queue) > 0) {
        
        // percorre a fila de tarefas dormindo e acorda apenas as que já podem acordar
        task_t *loop_current_task = (task_t *) sleeping_queue;
        for (int i = queue_size(sleeping_queue); i > 0; i--) {
            task_t *next_task = (task_t *) loop_current_task->next;
            if (next_task->wake_up_time <= systime()) {
                #ifdef DEBUG
                    printf("[dispatcher_body] Acordando tarefa %d\n", next_task->id);
                #endif
                task_awake(next_task, (task_t **) &sleeping_queue);
                continue;
            }
            loop_current_task = next_task;
        }

        /* se a fila de prontas estiver vazia, continua */
        if (queue_size(ready_queue) == 0) {
            continue;
        }
        // escolhe a próxima tarefa a executar
        #ifdef DEBUG
            printf("[dispatcher_body] Fila de prontas com %d tarefas\n", queue_size(ready_queue));
            printf("[dispatcher_body] Escolhendo próxima tarefa\n");
        #endif
        next_task = scheduler();

        // se houver uma próxima tarefa
        if (next_task) {
            // remove a tarefa da fila de prontas
            queue_remove(&ready_queue, (queue_t *) next_task);

            // atualiza o tempo de início da tarefa
            current_task_start_time = systime();

            // transfere o controle para a proxima tarefa
            task_switch(next_task);

            // atualiza o tempo de término da tarefa
            current_task_end_time = systime();

            // atualiza o tempo de processamento total da tarefa
            next_task->processor_time += current_task_end_time - current_task_start_time;

            // atualiza a prioridade dinâmica da tarefa
            next_task->prio_d = DEFAULT_PRIO;

            #ifdef DEBUG
                printf("[dispatcher_body] Resetando prioridade dinâmica da tarefa %d para %d\n", next_task->id, DEFAULT_PRIO);
            #endif

            switch (next_task->status) {
                case PRONTA: // 0
                    queue_append(&ready_queue, (queue_t *) next_task);
                    break;
                // se a tarefa foi suspensa, a coloca na fila de suspensas
                case SUSPENSA: // 2
                    break;
                // se a tarefa foi encerrada, a remove da fila de prontas
                case TERMINADA: // 3
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

/* CORE */
/* -------------------------------------------------------------------------- */

void ppos_init () {
    setvbuf(stdout, 0, _IONBF, 0);
    /* seta o clock do sistema pra 0 */
    clock = 0;

    /* Inicializa o tratador de sinais */
    action.sa_handler = tick_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("WARNING [ppos_init] Erro em sigaction: ");
        exit(1);
    }

    /* Ajusta valores do temporizador */
    timer.it_value.tv_usec = CLOCK_INITIAL_VALUE_US; // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec = CLOCK_INITIAL_VALUE_S; // primeiro disparo, em segundos
    timer.it_interval.tv_usec = CLOCK_INTERVAL_VALUE_US; // disparos subsequentes, em micro-segundos de cada intervalo
    timer.it_interval.tv_sec = CLOCK_INTERVAL_VALUE_S; // disparos subsequentes, em segundos de cada intervalo

    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("WARNING [ppos_init] Erro em itimer: ");
    }

    /* Inicializa o contexto da main */
    main_task.start_time = systime();
    getcontext(&main_task.context);
    current_task = &main_task;
    main_task.id = MAIN_ID;
    main_task.type = USER_TASK;
    main_task.activations++;
    main_task.status = PRONTA;
    task_setprio(&main_task, DEFAULT_PRIO);
    main_task.quantum = QUANTUM;
    main_task.processor_time = 0;
    main_task.tasks_waiting_for_conclusion = NULL;
    current_task = &main_task;

    /* Inicializa as filas */
    ready_queue = NULL;
    sleeping_queue = NULL;

    queue_append(&ready_queue, (queue_t *) &main_task);


    /* Inicializa a tarefa dispatcher */
    task_init(&dispatcher_task, (void *) dispatcher_body, NULL);
    dispatcher_task.type = SYSTEM_TASK;
    dispatcher_task.id = DISPATCHER_ID;

    #ifdef DEBUG
        printf("[dispatcher_body] Tarefa dispatcher criada\n");
    #endif
    

    #ifdef DEBUG
        printf("[ppos_init] PPos iniciado\n");
    #endif
    task_switch(&dispatcher_task);
}


/* TASK HANDLING */
/* -------------------------------------------------------------------------- */

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
    task->id = task_count++; // id da tarefa
    task->status = PRONTA; // 0: pronta, 1: rodando, 2: suspensa
    task_setprio(task, DEFAULT_PRIO); // prioridade da tarefa
    task->quantum = QUANTUM; // quantum da tarefa
    task->type = USER_TASK; // tipo da tarefa (0 = user task, 1 = system task)
    task->activations = 0; // numero de ativacoes da tarefa
    task->start_time = systime(); // tempo de inicio da tarefa
    task->processor_time = 0; // tempo de processamento da tarefa
    task->wake_up_time = 0; // tempo de acordar da tarefa

    /* Adiciona a task na fila de prontas */
    queue_append(&ready_queue, (queue_t *) task);

    /* inicializa a fila de tarefas esperando a conclusão da tarefa */
    task->tasks_waiting_for_conclusion = NULL;

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
    task->activations++; // incrementa o número de ativações da tarefa
    task->quantum = QUANTUM; // reseta o quantum da tarefa
    /* Troca o contexto */
    swapcontext(&old_task->context, &task->context);
    return 0;
}

void task_exit(int exit_code) {
    /* se o status de saída for diferente de 0, imprime erro */
    if (exit_code < 0) {
        perror("WARNING [task_exit] Erro na finalização da tarefa: ");
    }

    current_task->exit_code = exit_code;

    /* atualiza o tempo de término da tarefa */
    current_task->end_time = systime();

    printf("Task %d exit: execution time  %d ms, processor time  %d ms, %d activations\n", \
     task_id(),\
     current_task->end_time - current_task->start_time,\
     current_task->processor_time,\
     current_task->activations);

    current_task->status = TERMINADA;
    task_count--;
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
                printf("[task_exit] Restam %d tarefas\n", task_count);
            #endif

            /* acorda as tarefas esperando a conclusão da tarefa */
            task_t *task_waiting = (task_t *) current_task->tasks_waiting_for_conclusion;
            #ifdef DEBUG
                printf("[task_exit] Acordando %d tarefas esperando a conclusão da tarefa %d\n", queue_size(current_task->tasks_waiting_for_conclusion) , task_id());
            #endif
            while (queue_size(current_task->tasks_waiting_for_conclusion) > 0) {
                task_awake(task_waiting, (task_t **) &current_task->tasks_waiting_for_conclusion);
            }
            task_switch(&dispatcher_task);
            break;
    }
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

int task_id (){
    /* se a tarefa for nula, imprime erro */
    if (!current_task){
        perror("WARNING [task_id] Tarefa nula: ");
        return -1;
    }
    return current_task->id;
}

/* TASK HANDLING */
/* -------------------------------------------------------------------------- */

/* TASK PRIORITIES */
/* -------------------------------------------------------------------------- */

void task_setprio(task_t *task, int prio) {
    /* verificando limites */ 
    if (prio > LOW_PRIO) {
        #ifdef DEBUG
            printf("[task_setprio] Prioridade %d maior que o limite superior, setando para %d\n", prio, LOW_PRIO);
        #endif
        prio = LOW_PRIO;
    }

    if (prio < HIGH_PRIO) {
        #ifdef DEBUG
            printf("[task_setprio] Prioridade %d menor que o limite inferior, setando para %d\n", prio, HIGH_PRIO);
        #endif
        prio = HIGH_PRIO;
    }

    /* se a tarefa for nula, imprime erro */
    if (task == NULL){
        #ifdef DEBUG
            printf("[task_setprio] Tarefa nula, setando prioridade para a tarefa atual\n");
        #endif
        current_task->prio_e = prio;
    }

    #ifdef DEBUG
        printf("[task_setprio] Setando prioridade da tarefa %d para %d\n", task->id, prio);
    #endif
    task->prio_e = prio;
}

int task_getprio(task_t *task) {
    /* se a tarefa for nula, retorna a prioridade da tarefa atual */
    if (task == NULL)
        return current_task->prio_e;

    return task->prio_e;
}

/* TASK PRIORITIES */
/* -------------------------------------------------------------------------- */



/* TASK SUSPENSION */
/* -------------------------------------------------------------------------- */

void task_suspend (task_t **queue) {
    if (queue == NULL) {
        perror("WARNING [task_suspend] Fila nula: ");
        return;
    }

    if (current_task->status == TERMINADA) {
        perror("WARNING [task_suspend] Tarefa encerrada: ");
        return;
    }

    /* se a tarefa estiver rodando, muda seu status e insere na fila suspensa */
    #ifdef DEBUG
        printf("[task_suspend] Tarefa %d suspensa\n", current_task->id);
    #endif
    if (current_task->id == MAIN_ID && queue_size(ready_queue) > 0 && current_task->status == RODANDO) {
        queue_remove((queue_t **) &ready_queue, (queue_t *) current_task);
    }
    queue_append((queue_t **) queue, (queue_t *) current_task);
    task_switch(&dispatcher_task);
    
    return;
}

void task_awake(task_t* task, task_t **queue) {
    if (queue == NULL) {
        perror("WARNING [task_awake] Fila nula: ");
        return;
    }

    if (task == NULL) {
        perror("WARNING [task_awake] Tarefa nula: ");
        return;
    }

    /* remove a tarefa da fila de suspensas e insere na fila de prontas */
    task->status = PRONTA;
    queue_remove((queue_t **) queue, (queue_t *) task);
    queue_append((queue_t **) &ready_queue, (queue_t *) task);
    #ifdef DEBUG
        printf("[task_awake] Tarefa %d acordada em %d\n", task->id, systime());
        printf("[task_awake] Tarefa %d adicionada na fila de prontas\n", task->id);
    #endif

    return;
}

int task_wait(task_t *task) {
    if (task == NULL) {
        return -1;
    }

    if (task->status == TERMINADA) {
        return task->exit_code;
    }

    #ifdef DEBUG
        printf("[task_wait] Tarefa %d esperando a conclusão da tarefa %d\n", current_task->id, task->id);
    #endif
    
    current_task->status = SUSPENSA;
    task_suspend((task_t **) &task->tasks_waiting_for_conclusion);


    return task->exit_code;
}

void task_sleep(int t) {
    if (t < 0) {
        perror("WARNING [task_sleep] Tempo inválido: ");
        return;
    }

    current_task->status = DORMINDO;
    current_task->wake_up_time = systime() + t;
    #ifdef DEBUG
        printf("[task_sleep] Tarefa %d dormindo por %d ms\n", current_task->id, t);
        printf("[task_sleep] Acordará em %d ms\n", current_task->wake_up_time);
    #endif
    task_suspend((task_t **) &sleeping_queue);

}

/* TASK SUSPENSION */
/* -------------------------------------------------------------------------- */

/* SEMAPHORE */
/* -------------------------------------------------------------------------- */

void enter_cs(int *lock) {
    #ifdef DEBUG
        printf("[enter_cs] Tentando entrar na seção crítica\n");
    #endif
    while (__sync_fetch_and_or(lock, 1));
}

void leave_cs(int *lock) {
    #ifdef DEBUG
        printf("[leave_cs] Saindo da seção crítica\n");
    #endif
    (*lock) = 0;
}

int sem_init(semaphore_t *s, int value) {
    if (s == NULL) {
        perror("WARNING [sem_init] Semáforo nulo: ");
        return -1;
    }


    s->counter = value;
    s->queue = NULL;
    s->lock = 0;
    
    #ifdef DEBUG
        printf("[sem_init] Inicializando semáforo com valor %d\n", value);
    #endif


    return 0;
}

int sem_down(semaphore_t *s) {

    enter_cs(&s->lock);
    if (s == NULL) {
        perror("WARNING [sem_down] Semáforo nulo: ");
        return -1;
    }

    s->counter--;

    #ifdef DEBUG
        printf("[sem_down] Task: %d. Decrementando semáforo para %d\n", task_id(), s->counter);
    #endif

    if (s->counter < 0) {
        #ifdef DEBUG
            printf("[sem_down] Tarefa %d bloqueada\n", task_id());
        #endif
        task_suspend((task_t **) &s->queue);
    }
    leave_cs(&s->lock);

    return 0;
}

int sem_up(semaphore_t *s) {
    enter_cs(&s->lock);
    if (s == NULL) {
        perror("WARNING [sem_up] Semáforo nulo: ");
        return -1;
    }

    s->counter++;
    #ifdef DEBUG
        printf("[sem_up] Incrementando semáforo para %d\n", s->counter);
    #endif

    if (s->counter <= 0) {
        #ifdef DEBUG
            printf("[sem_up] Tarefa %d acordada\n", ((task_t *) s->queue)->id);
        #endif
        task_awake((task_t *) s->queue, (task_t **) &s->queue);
    }
    leave_cs(&s->lock);

    return 0;
}

int sem_destroy(semaphore_t *s) {
    if (s == NULL) {
        perror("WARNING [sem_destroy] Semáforo nulo: ");
        return -1;
    }

    while (queue_size(s->queue) > 0) {

        /* change task exit code */
        task_t *task = (task_t *) s->queue;
        task->exit_code = -1;
        task_awake((task_t *) s->queue, (task_t **) &s->queue);
    }

    s->counter = 0;
    s->queue = NULL;
    s = NULL;

    return 0;
}


/* SEMAPHORE */
/* -------------------------------------------------------------------------- */

/* MESSAGE */
/* -------------------------------------------------------------------------- */

int mqueue_init(mqueue_t *queue, int max_msgs, int msg_size) {
    if (queue == NULL) {
        perror("WARNING [mqueue_init] Fila de mensagens nula: ");
        return -1;
    }

    queue->buffer = malloc(max_msgs * msg_size);
    if (queue->buffer == NULL) {
        perror("WARNING [mqueue_init] Erro na alocação do buffer: ");
        return -1;
    }

    queue->max_msgs = max_msgs;
    queue->msg_size = msg_size;
    queue->buffer_top = 0;

    sem_init(&queue->s_buffer, 1);
    sem_init(&queue->s_vaga, max_msgs);
    sem_init(&queue->s_item, 0);

    #ifdef DEBUG
        printf("[mqueue_init] Fila de mensagens inicializada com %d mensagens de tamanho %d\n", max_msgs, msg_size);
    #endif

    return 0;
}

int mqueue_send(mqueue_t* queue, void *msg) {
    if (queue == NULL) {
        perror("WARNING [mqueue_send] Fila de mensagens nula: ");
        return -1;
    }


    sem_down(&queue->s_vaga);
    sem_down(&queue->s_buffer);


    int offset = queue->buffer_top * queue->msg_size;
    memcpy((void *) (queue->buffer + offset), msg, queue->msg_size);
    queue->buffer_top++;

    sem_up(&queue->s_buffer);

    sem_up(&queue->s_item);

    return 0;
}

int mqueue_recv(mqueue_t *queue, void *msg) {
    if (queue == NULL) {
        perror("WARNING [mqueue_recv] Fila de mensagens nula: ");
        return -1;
    }


    sem_down(&queue->s_item);
    sem_down(&queue->s_buffer);


    memcpy(msg, (void *) queue->buffer, queue->msg_size);

    /* move todas as mensagens uma posição para trás */
    for (int i = 0; i < queue->buffer_top; i++) {
        int offset = i * queue->msg_size;
        memcpy((void *) queue->buffer + offset, (void *) queue->buffer + offset + queue->msg_size, queue->msg_size);
    }
    queue->buffer_top--;


    sem_up(&queue->s_buffer);
    sem_up(&queue->s_vaga);


    return 0;
}

int mqueue_destroy(mqueue_t *queue) {
    if (queue == NULL) {
        perror("WARNING [mqueue_destroy] Fila de mensagens nula: ");
        return -1;
    }

    sem_destroy(&queue->s_vaga);
    sem_destroy(&queue->s_item);
    sem_destroy(&queue->s_buffer);

    free(queue->buffer);

    queue->buffer = NULL;
    queue->max_msgs = 0;
    queue->msg_size = 0;
    queue->buffer_top = 0;
    queue = NULL;

    return 0;
}

int mqueue_msgs(mqueue_t *queue) {
    if (queue == NULL) {
        perror("WARNING [mqueue_msgs] Fila de mensagens nula: ");
        return -1;
    }

    return queue->buffer_top;
}


/* MESSAGE */
/* -------------------------------------------------------------------------- */