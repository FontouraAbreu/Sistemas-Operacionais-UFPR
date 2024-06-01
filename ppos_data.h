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
#include <string.h>
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
} task_t ;

// estrutura que define um semáforo
typedef struct
{
  int counter; // contador do semáforo
  queue_t *queue; // fila de tarefas bloqueadas no semáforo
  int lock; // lock do semáforo
  short valid; // semáforo valido
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
  int max_msgs; // capacidade da fila
  int msg_size; // tamanho de cada mensagem
  semaphore_t s_buffer; // semáforo do buffer
  semaphore_t s_vaga; // semáforo da vaga
  semaphore_t s_item; // semáforo do item
  void *buffer; // ponteiro para o buffer
  int buffer_top; // topo do buffer
  short valid; // fila de mensagens valida
} mqueue_t ;

#endif

