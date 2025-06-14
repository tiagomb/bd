#include "serializability.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void checkSerializability(schedule_t *schedule) {
    if (schedule == NULL || schedule->transactions == NULL || schedule->transactions->head == NULL) {
        if (schedule) { strcpy(schedule->serializable, "SS"); strcpy(schedule->equivalent, "SV"); }
        return;
    }

    int maxTid = -1;
    bool tidsEnvolved[MAX_TID_VAL] = {false};
    int uniqueTids[MAX_TID_VAL];
    int numUniqueTids = 0;
    
    transaction_t* opNode = schedule->transactions->head;
    while (opNode != NULL) {
        if (opNode->id >= 0 && opNode->id < MAX_TID_VAL) {
            if (!tidsEnvolved[opNode->id]) {
                tidsEnvolved[opNode->id] = true;
                if (numUniqueTids < MAX_TID_VAL) {
                    uniqueTids[numUniqueTids++] = opNode->id;
                }
            }
            if (opNode->id > maxTid) maxTid = opNode->id;
        }
        opNode = opNode->next;
    }

    if (maxTid == -1) {
        strcpy(schedule->serializable, "SS"); strcpy(schedule->equivalent, "SV"); return;
    }
    
    bool adj[MAX_TID_VAL][MAX_TID_VAL];
    buildConflictGraph(schedule, maxTid, adj);
    
    if (!hasCycle(maxTid, tidsEnvolved, adj)) {
        strcpy(schedule->serializable, "SS");
        strcpy(schedule->equivalent, "SV");
        return;
    }
    
    strcpy(schedule->serializable, "NS");
    
    if (numUniqueTids > MAX_TID_VAL) {
         fprintf(stderr, "Aviso: Excesso de transacoes unicas para o teste de visao por permutacao (limite: %d).\n", MAX_TID_VAL);
         strcpy(schedule->equivalent, "NV");
         return;
    }

    transaction_list_t* groupedOps[MAX_TID_VAL] = {NULL};
    opNode = schedule->transactions->head;
    while(opNode != NULL) {
        int tid = opNode->id;
        if (tid >= 0 && tid < MAX_TID_VAL) {
            if (groupedOps[tid] == NULL) {
                groupedOps[tid] = (transaction_list_t*)malloc(sizeof(transaction_list_t));
                groupedOps[tid]->head = NULL;
                groupedOps[tid]->size = 0;
            }
            transaction_t* opCopy = (transaction_t*)malloc(sizeof(transaction_t));
            *opCopy = *opNode;
            opCopy->next = NULL;

            // Anexa a cópia à lista agrupada correta
            if (groupedOps[tid]->head == NULL) {
                groupedOps[tid]->head = opCopy;
            } else {
                transaction_t* tail = groupedOps[tid]->head;
                while(tail->next != NULL) tail = tail->next;
                tail->next = opCopy;
            }
            groupedOps[tid]->size++;
        }
        opNode = opNode->next;
    }

    if (checkAllSerialSchedules(schedule, uniqueTids, 0, numUniqueTids - 1, groupedOps)) {
        strcpy(schedule->equivalent, "SV");
    } else {
        strcpy(schedule->equivalent, "NV");
    }

    for (int i = 0; i < MAX_TID_VAL; ++i) {
        if(groupedOps[i] != NULL) {
            transaction_t* aux = groupedOps[i]->head;
            while(aux != NULL) {
                transaction_t* temp = aux;
                aux = aux->next;
                free(temp);
            }
            free(groupedOps[i]);
        }
    }
}

int getFinalWriteBeforeRead(int readTimestamp, transaction_list_t* opList) {
    int writerTid = 0;
    transaction_t* readNode = opList->head;
    while(readNode && readNode->timestamp != readTimestamp) {
        readNode = readNode->next;
    }
    if (!readNode) return -1; // Não deveria acontecer

    transaction_t* curr = opList->head;
    while(curr != NULL && curr->timestamp != readTimestamp) {
        if(curr->op == 'W' && curr->atrib == readNode->atrib) {
            writerTid = curr->id;
        }
        curr = curr->next;
    }
    return writerTid;
}

int getFinalWriteTid(char atrib, transaction_list_t* opList) {
    int writerTid = -1;
    transaction_t* curr = opList->head;
    while(curr != NULL) {
        if(curr->op == 'W' && curr->atrib == atrib) {
            writerTid = curr->id;
        }
        curr = curr->next;
    }
    return writerTid;
}

bool areSchedulesViewEquivalent(schedule_t* s1, schedule_t* s2) {
    bool attributes[MAX_ATTRIB_VAL] = {false};
    transaction_t* op = s1->transactions->head;
    while(op != NULL) {
        attributes[(int)op->atrib] = true;
        op = op->next;
    }
    op = s1->transactions->head;
    while(op != NULL) {
        if(op->op == 'R') {
            if (getFinalWriteBeforeRead(op->timestamp, s1->transactions) != getFinalWriteBeforeRead(op->timestamp, s2->transactions)) {
                return false;
            }
        }
        op = op->next;
    }
    for(int i=0; i<MAX_ATTRIB_VAL; ++i) {
        if(attributes[i]) {
            if (getFinalWriteTid((char)i, s1->transactions) != getFinalWriteTid((char)i, s2->transactions)) {
                return false;
            }
        }
    }
    return true;
}

bool checkAllSerialSchedules(schedule_t* originalSchedule, int *uniqueTids, int start, int end, transaction_list_t **groupedOps) {
    if (start == end) {
        transaction_list_t* serialTransactions = (transaction_list_t*)malloc(sizeof(transaction_list_t));
        serialTransactions->head = NULL;
        serialTransactions->size = 0;
        transaction_t* serial_tail = NULL;

        for (int i = 0; i <= end; i++) {
            int currTid = uniqueTids[i];
            transaction_list_t* sourceList = groupedOps[currTid];
            if (sourceList) {
                transaction_t* opCopy = sourceList->head;
                while(opCopy) {
                    transaction_t* newOp = (transaction_t*)malloc(sizeof(transaction_t));
                    *newOp = *opCopy;
                    newOp->next = NULL;

                    if (serialTransactions->head == NULL) {
                        serialTransactions->head = newOp;
                        serial_tail = newOp;
                    } else {
                        serial_tail->next = newOp;
                        serial_tail = newOp;
                    }
                    serialTransactions->size++;
                    opCopy = opCopy->next;
                }
            }
        }
        
        schedule_t temp;
        temp.transactions = serialTransactions;
        bool isEquivalent = areSchedulesViewEquivalent(originalSchedule, &temp);

        transaction_t* aux = serialTransactions->head;
        while(aux != NULL) {
            transaction_t* temp = aux;
            aux = aux->next;
            free(temp);
        }
        free(serialTransactions);
        if (isEquivalent) return true;

    } else {
        for (int i = start; i <= end; i++) {
            int temp = uniqueTids[start];
            uniqueTids[start] = uniqueTids[i];
            uniqueTids[i] = temp;
            if (checkAllSerialSchedules(originalSchedule, uniqueTids, start + 1, end, groupedOps)) {
                return true;
            }
            temp = uniqueTids[start];
            uniqueTids[start] = uniqueTids[i];
            uniqueTids[i] = temp;
        }
    }
    return false;
}

void buildConflictGraph(schedule_t *schedule, int maxTid, bool adj[][MAX_TID_VAL]) {
    for(int i = 0; i <= maxTid; ++i) 
        for(int j = 0; j <= maxTid; ++j) 
            adj[i][j] = false;
    transaction_t* op1;
    transaction_t* op2;
    for (op1 = schedule->transactions->head; op1 != NULL; op1 = op1->next) {
        if (op1->op != 'R' && op1->op != 'W') continue;
        for (op2 = op1->next; op2 != NULL; op2 = op2->next) {
            if (op2->op != 'R' && op2->op != 'W') continue;
            if (op1->id != op2->id && op1->atrib == op2->atrib && (op1->op == 'W' || op2->op == 'W')) {
                adj[op1->id][op2->id] = true;
            }
        }
    }
}

bool hasCycle(int maxTid, bool *tidsEnvolved, bool adj[][MAX_TID_VAL]) {
    bool visitedNodes[MAX_TID_VAL] = {false};
    bool stackNodes[MAX_TID_VAL] = {false};
    for (int i = 0; i <= maxTid; ++i) {
        if (tidsEnvolved[i] && !visitedNodes[i]) {
            if (cycleDetectionUtil(i, maxTid, adj, visitedNodes, stackNodes, tidsEnvolved)) {
                return true;
            }
        }
    }
    return false;
}

bool cycleDetectionUtil(int tid, int maxTid, bool adj[][MAX_TID_VAL], bool *visitedNodes, bool *stackNodes, bool *tidsEnvolved) {
    if (!tidsEnvolved[tid]) return false;
    visitedNodes[tid] = true;
    stackNodes[tid] = true;
    for (int vTid = 0; vTid <= maxTid; ++vTid) {
        if (tidsEnvolved[vTid] && adj[tid][vTid]) {
            if (!visitedNodes[vTid]) {
                if (cycleDetectionUtil(vTid, maxTid, adj, visitedNodes, stackNodes, tidsEnvolved)) 
                    return true;
            } else if (stackNodes[vTid]) 
                return true;
        }
    }
    stackNodes[tid] = false;
    return false;
}