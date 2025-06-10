#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_TID_VAL 100
#define MAX_ATTRIB_VAL 128
#define MAX_UNIQUE_TIDS_FOR_PERMUTATION 10 // Limite prático para o método de permutação

// --- Struct Definitions (sem alterações) ---
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


// --- readEntries (sem alterações) ---
void readEntries(schedule_list_t *schedules){
    schedules->head = NULL;
    schedules->size = 0;
    schedule_t *currentSchedule = NULL;
    schedule_t *schedulesTail = NULL;
    int scheduleCounter = 1;
    int scheduleCommits = 0;
    transaction_t *transactionsTail= NULL;
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
            transactionsTail= NULL;
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
            transactionsTail= newTransaction;
        } else {
            transactionsTail->next = newTransaction;
            transactionsTail= newTransaction;
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

// --- Funções para a Abordagem Tradicional (Permutação) - CORRIGIDAS ---

// --- MUDANÇA AQUI: Nova função que usa timestamp em vez de ponteiro ---
int get_read_from_source_by_timestamp(int read_op_timestamp, transaction_list_t* op_list) {
    int writer_tid = 0; // 0 representa o valor inicial
    transaction_t* current_op = op_list->head;
    
    // Percorre a lista até a operação de leitura específica
    while(current_op != NULL && current_op->timestamp != read_op_timestamp) {
        // Enquanto procura, rastreia a escrita mais recente no mesmo atributo
        if(current_op->op == 'W') {
            // Encontra a operação de leitura na lista para obter o atributo
            transaction_t* read_op_node = op_list->head;
            while(read_op_node && read_op_node->timestamp != read_op_timestamp) {
                read_op_node = read_op_node->next;
            }
            if (read_op_node && current_op->atrib == read_op_node->atrib) {
                 writer_tid = current_op->id;
            }
        }
        current_op = current_op->next;
    }
    return writer_tid;
}

// Função auxiliar para encontrar a transação que fez a última escrita em um atributo
int get_final_write_tid(char atrib, transaction_list_t* op_list) {
    int writer_tid = -1;
    transaction_t* current_op = op_list->head;
    while(current_op != NULL) {
        if(current_op->op == 'W' && current_op->atrib == atrib) {
            writer_tid = current_op->id;
        }
        current_op = current_op->next;
    }
    return writer_tid;
}

// --- MUDANÇA AQUI: Compara agendamentos usando a nova função auxiliar ---
bool are_schedules_view_equivalent(schedule_t* s1, schedule_t* s2) {
    bool attributes_present[MAX_ATTRIB_VAL] = {false};
    transaction_t* op = s1->transactions->head;
    while(op != NULL) {
        attributes_present[(int)op->atrib] = true;
        op = op->next;
    }

    // 1. Verifica a condição de Leitura-De (Read-From)
    op = s1->transactions->head;
    while(op != NULL) {
        if(op->op == 'R') {
            int op_timestamp = op->timestamp;
            int source_s1 = get_read_from_source_by_timestamp(op_timestamp, s1->transactions);
            int source_s2 = get_read_from_source_by_timestamp(op_timestamp, s2->transactions);
            if (source_s1 != source_s2) {
                return false;
            }
        }
        op = op->next;
    }

    // 2. Verifica a condição de Escrita Final (Final Write)
    for(int i=0; i<MAX_ATTRIB_VAL; ++i) {
        if(attributes_present[i]) {
            if (get_final_write_tid((char)i, s1->transactions) != get_final_write_tid((char)i, s2->transactions)) {
                return false;
            }
        }
    }
    return true;
}


// Função recursiva para gerar permutações e testá-las
bool check_all_serial_schedules(schedule_t* original_schedule, int unique_tids[], int start, int end) {
    if (start == end) {
        transaction_list_t* serial_transactions = (transaction_list_t*)malloc(sizeof(transaction_list_t));
        serial_transactions->head = NULL;
        serial_transactions->size = 0;
        transaction_t* serial_tail = NULL;
        for (int i = 0; i <= end; i++) {
            int current_tid = unique_tids[i];
            transaction_t* op = original_schedule->transactions->head;
            while(op != NULL) {
                if (op->id == current_tid) {
                    transaction_t* new_op = (transaction_t*)malloc(sizeof(transaction_t));
                    *new_op = *op;
                    new_op->next = NULL;
                    if (serial_transactions->head == NULL) {
                        serial_transactions->head = new_op;
                        serial_tail = new_op;
                    } else {
                        serial_tail->next = new_op;
                        serial_tail = new_op;
                    }
                    serial_transactions->size++;
                }
                op = op->next;
            }
        }
        schedule_t temp_serial_schedule;
        temp_serial_schedule.transactions = serial_transactions;
        bool is_equivalent = are_schedules_view_equivalent(original_schedule, &temp_serial_schedule);
        transaction_t* op_to_free = serial_transactions->head;
        while(op_to_free != NULL) {
            transaction_t* temp = op_to_free;
            op_to_free = op_to_free->next;
            free(temp);
        }
        free(serial_transactions);
        if (is_equivalent) return true;
    } else {
        for (int i = start; i <= end; i++) {
            int temp = unique_tids[start];
            unique_tids[start] = unique_tids[i];
            unique_tids[i] = temp;
            if (check_all_serial_schedules(original_schedule, unique_tids, start + 1, end)) {
                return true;
            }
            temp = unique_tids[start];
            unique_tids[start] = unique_tids[i];
            unique_tids[i] = temp;
        }
    }
    return false;
}


// --- Funções principais e de limpeza (sem alterações) ---
bool cycleDetection(int tid, int maxTid, bool adj[][MAX_TID_VAL], bool visitedNodes[], bool stackNodes[], bool tidsEnvolved[]);
void buildConflictGraph(schedule_t *schedule, int maxTid, bool adj[][MAX_TID_VAL]);

void checkSerializability(schedule_t *schedule) {
    if (schedule == NULL || schedule->transactions == NULL || schedule->transactions->head == NULL) {
        if (schedule) { strcpy(schedule->serializable, "SS"); strcpy(schedule->equivalent, "SV"); }
        return;
    }
    int maxTid = -1;
    bool tidsEnvolved[MAX_TID_VAL] = {false};
    int unique_tids[MAX_UNIQUE_TIDS_FOR_PERMUTATION];
    int num_unique_tids = 0;
    transaction_t* opNode = schedule->transactions->head;
    while (opNode != NULL) {
        if (opNode->id >= 0 && opNode->id < MAX_TID_VAL) {
            if (!tidsEnvolved[opNode->id]) {
                tidsEnvolved[opNode->id] = true;
                if (num_unique_tids < MAX_UNIQUE_TIDS_FOR_PERMUTATION) {
                    unique_tids[num_unique_tids++] = opNode->id;
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
    bool visitedNodes[MAX_TID_VAL] = {false};
    bool stackNodes[MAX_TID_VAL] = {false};
    bool hasCycle = false;
    for (int i = 0; i <= maxTid; ++i) {
        if (tidsEnvolved[i] && !visitedNodes[i]) {
            if (cycleDetection(i, maxTid, adj, visitedNodes, stackNodes, tidsEnvolved)) {
                hasCycle = true;
                break;
            }
        }
    }
    strcpy(schedule->serializable, hasCycle ? "NS" : "SS");
    if (!hasCycle) {
        strcpy(schedule->equivalent, "SV");
        return;
    }
    if (num_unique_tids > MAX_UNIQUE_TIDS_FOR_PERMUTATION) {
         fprintf(stderr, "Aviso: Excesso de transacoes unicas para o teste de visao por permutacao (limite: %d). Agendamento %d sera marcado como NV.\n", MAX_UNIQUE_TIDS_FOR_PERMUTATION, schedule->id);
         strcpy(schedule->equivalent, "NV");
         return;
    }
    if (check_all_serial_schedules(schedule, unique_tids, 0, num_unique_tids - 1)) {
        strcpy(schedule->equivalent, "SV");
    } else {
        strcpy(schedule->equivalent, "NV");
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
                sprintf(temp, "%d ", op->id);
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
    currentSchedule = allSchedules.head;
    while (currentSchedule != NULL) {
        schedule_t *tempSchedule = currentSchedule;
        currentSchedule = currentSchedule->next;
        transaction_t *current_op = tempSchedule->transactions->head;
        while (current_op != NULL) {
            transaction_t *temp_op = current_op;
            current_op = current_op->next;
            free(temp_op);
        }
        free(tempSchedule->transactions);
        free(tempSchedule);
    }
    return 0;
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

bool cycleDetection(int tid, int maxTid, bool adj[][MAX_TID_VAL], bool visitedNodes[], bool stackNodes[], bool tidsEnvolved[]) {
    if (!tidsEnvolved[tid]) return false;
    visitedNodes[tid] = true;
    stackNodes[tid] = true;
    for (int vTid = 0; vTid <= maxTid; ++vTid) {
        if (tidsEnvolved[vTid] && adj[tid][vTid]) {
            if (!visitedNodes[vTid]) {
                if (cycleDetection(vTid, maxTid, adj, visitedNodes, stackNodes, tidsEnvolved)) return true;
            } else if (stackNodes[vTid]) return true;
        }
    }
    stackNodes[tid] = false;
    return false;
}