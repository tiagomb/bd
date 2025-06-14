#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdbool.h>

#define MAX_TID_VAL 100
#define MAX_ATTRIB_VAL 128

typedef struct transaction_s {
    int timestamp;
    int id;
    char op;
    char atrib;
    struct transaction_s *next;
} transaction_t;

typedef struct {
    int size;
    transaction_t *head;
} transaction_list_t;

typedef struct schedule_s {
    int id;
    transaction_list_t *transactions;
    struct schedule_s *next;
    int size;
    char serializable[3];
    char equivalent[3];
} schedule_t;

typedef struct {
    int size;
    schedule_t *head;
} schedule_list_t;

// Lê transações da entrada padrão e as agrupa em uma lista de agendamentos.
void readEntries(schedule_list_t *schedules);

#endif // SCHEDULE_H