#ifndef SERIALIZABILITY_H
#define SERIALIZABILITY_H

#include "schedule.h"

#define MAX_UNIQUE_TIDS 15

// Verifica se um agendamento é serializável por conflito e se é equivalente por visão.
void checkSerializability(schedule_t *schedule);

// Obtém o ID da transação que fez a última escrita antes de uma leitura específica.
int getFinalWriteBeforeRead(int read_op_timestamp, transaction_list_t* op_list);

// Recebe 2 agendamentos e verifica se são equivalentes por visão.
bool areSchedulesViewEquivalent(schedule_t* s1, schedule_t* s2);

// Gera todas as permutações de transações e verifica se algum agendamento serial é equivalente ao original.
bool checkAllSerialSchedules(schedule_t* original_schedule, int *unique_tids, int start, int end, transaction_list_t **grouped_ops);

// Constrói o grafo de conflitos a partir do agendamento.
void buildConflictGraph(schedule_t *schedule, int maxTid, bool adj[][MAX_TID_VAL]);

// Verifica se o grafo de conflitos contém ciclos, indicando que o agendamento não é serializável.
bool hasCycle(int maxTid, bool *tidsEnvolved, bool adj[][MAX_TID_VAL]);

// Função auxiliar para detectar ciclos no grafo de conflitos.
bool cycleDetectionUtil(int tid, int maxTid, bool adj[][MAX_TID_VAL], bool *visitedNodes, bool *stackNodes, bool *tidsEnvolved);

#endif // SERIALIZABILITY_H