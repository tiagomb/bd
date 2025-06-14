#include "schedule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void readEntries(schedule_list_t *schedules) {
    schedules->head = NULL;
    schedules->size = 0;
    schedule_t *currentSchedule = NULL;
    schedule_t *schedulesTail = NULL;
    int scheduleCounter = 1;
    int scheduleCommits = 0;
    transaction_t *transactionsTail = NULL;
    char buffer[256];

    bool transactionsInSchedule[MAX_TID_VAL] = {false};
    int uniqueTransactionsCount = 0;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (strspn(buffer, " \t\n\r") == strlen(buffer)) continue;

        transaction_t *newTransaction = (transaction_t*)malloc(sizeof(transaction_t));
        newTransaction->next = NULL;
        sscanf(buffer, "%d %d %c %c", &newTransaction->timestamp, &newTransaction->id, &newTransaction->op, &newTransaction->atrib);

        if (currentSchedule == NULL) {
            currentSchedule = (schedule_t*)malloc(sizeof(schedule_t));
            currentSchedule->id = scheduleCounter++;
            currentSchedule->size = 0;
            currentSchedule->next = NULL;
            currentSchedule->serializable[0] = '\0';
            currentSchedule->equivalent[0] = '\0';
            currentSchedule->transactions = (transaction_list_t*)malloc(sizeof(transaction_list_t));
            currentSchedule->transactions->head = NULL;
            currentSchedule->transactions->size = 0;
            transactionsTail = NULL;
            scheduleCommits = 0;
            uniqueTransactionsCount = 0;
            memset(transactionsInSchedule, false, sizeof(transactionsInSchedule));

            if (schedules->head == NULL) {
                schedules->head = currentSchedule;
                schedulesTail = currentSchedule;
            } else {
                schedulesTail->next = currentSchedule;
                schedulesTail = currentSchedule;
            }
            schedules->size++;
        }

        if (currentSchedule->transactions->head == NULL) {
            currentSchedule->transactions->head = newTransaction;
            transactionsTail = newTransaction;
        } else {
            transactionsTail->next = newTransaction;
            transactionsTail = newTransaction;
        }
        currentSchedule->transactions->size++;
        currentSchedule->size = currentSchedule->transactions->size;

        int transactionID = newTransaction->id;
        if (transactionID >= 0 && transactionID < MAX_TID_VAL) {
            if (!transactionsInSchedule[transactionID]) {
                transactionsInSchedule[transactionID] = true;
                uniqueTransactionsCount++;
            }
        }

        if (newTransaction->op == 'C') {
            scheduleCommits++;
            if (scheduleCommits == uniqueTransactionsCount && uniqueTransactionsCount > 0) {
                currentSchedule = NULL;
            }
        }
    }
}