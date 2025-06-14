#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "schedule.h"
#include "serializability.h"

void freeMemory(schedule_list_t *schedules) {
    schedule_t *currentSchedule = schedules->head;
    while (currentSchedule != NULL) {
        schedule_t *tempSchedule = currentSchedule;
        currentSchedule = currentSchedule->next;
        
        transaction_t *currentOp = tempSchedule->transactions->head;
        while (currentOp != NULL) {
            transaction_t *temp_op = currentOp;
            currentOp = currentOp->next;
            free(temp_op);
        }
        free(tempSchedule->transactions);
        free(tempSchedule);
    }
}

int main() {
    schedule_list_t allSchedules;
    readEntries(&allSchedules);

    schedule_t *currentSchedule = allSchedules.head;
    while (currentSchedule != NULL) {
        checkSerializability(currentSchedule);

        char transactions[1024] = "";
        bool tidListed[MAX_TID_VAL] = {false};
        transaction_t *op = currentSchedule->transactions->head;
        while(op != NULL) {
            if (op->id >= 0 && op->id < MAX_TID_VAL && !tidListed[op->id]) {
                char temp[10];
                sprintf(temp, "%d,", op->id);
                strcat(transactions, temp);
                tidListed[op->id] = true;
            }
            op = op->next;
        }
        if(strlen(transactions) > 0) {
            transactions[strlen(transactions) - 1] = '\0';
        }
        
        printf("%d %s %s %s\n", currentSchedule->id, transactions, currentSchedule->serializable, currentSchedule->equivalent);
        
        currentSchedule = currentSchedule->next;
    }

    freeMemory(&allSchedules);
    return 0;
}