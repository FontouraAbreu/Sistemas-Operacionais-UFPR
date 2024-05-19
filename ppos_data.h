// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include <signal.h>
#include <sys/time.h> 
#include <unistd.h>
#include "queue.h"



// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  struct queue_t *tasks_waiting_for_conclusion; // ponteiro para a fila de tarefas esperando a conclusao da tarefa
  int id ;				    // identificador da tarefa
  ucontext_t context ;// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  int prio_e;        // prioridade estatica da tarefa
  int prio_d;        // prioridade dinamica da tarefa
  unsigned int quantum; // quantum restante da tarefa
  unsigned int type; // tipo da tarefa (0 = user task, 1 = system task)
  unsigned int activations; // numero de ativacoes da tarefa
  unsigned int start_time; // tempo de inicio da tarefa
  unsigned int end_time; // tempo de termino da tarefa
  unsigned int processor_time; // tempo de processamento da tarefa
  int exit_code; // codigo de saida da tarefa
  unsigned int wake_up_time; // tempo de acordar da tarefa
  // ... (outros campos serão adicionados mais tarde)
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

