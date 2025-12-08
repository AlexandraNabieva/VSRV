#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define NUM_EMPLOYEES 2

struct employee {
    int number;
    int id;
    char first_name[20];
    char last_name[30];
    char department[30];
    int room_number;
};

struct employee employees[] = {
    { 1, 12345678, "danny", "coresh", "Accounting", 101},
    { 2, 87654321, "misha", "levyn", "Programmers", 202}
};

struct employee employee_of_the_day;

void copy_employee(struct employee* from, struct employee* to)
{
    to->number = from->number;
    to->id = from->id;
    strcpy(to->first_name, from->first_name);
    strcpy(to->last_name, from->last_name);
    strcpy(to->department, from->department);
    to->room_number = from->room_number;
}

void* do_loop(void* data)
{
    int my_num = *((int*)data);
    
    while (1) {
        copy_employee(&employees[my_num-1], &employee_of_the_day);
    }
}

int main(int argc, char* argv[])
{
    int i;
    int thr_id1, thr_id2;
    pthread_t p_thread1, p_thread2;
    int num1 = 1, num2 = 2;
    
    struct employee eotd;
    struct employee* worker;

    copy_employee(&employees[0], &employee_of_the_day);

    printf("Starting program WITHOUT mutex synchronization\n");
    printf("Expected: consistent employee data\n");
    printf("Actual: may have race conditions and data corruption\n\n");

    thr_id1 = pthread_create(&p_thread1, NULL, do_loop, (void*)&num1);
    thr_id2 = pthread_create(&p_thread2, NULL, do_loop, (void*)&num2);

    for (i=0; i<60000; i++) {
        copy_employee(&employee_of_the_day, &eotd);
        worker = &employees[eotd.number-1];

        if (eotd.id != worker->id) {
            printf("ERROR: mismatching 'id', %d != %d (loop '%d')\n", 
                   eotd.id, worker->id, i);
            printf("RACE CONDITION DETECTED: Threads interrupted during data update\n");
            exit(0);
        }

        if (strcmp(eotd.first_name, worker->first_name) != 0) {
            printf("ERROR: mismatching 'first_name', %s != %s (loop '%d')\n", 
                   eotd.first_name, worker->first_name, i);
            printf("RACE CONDITION DETECTED: Partial string copy detected\n");
            exit(0);
        }

        if (strcmp(eotd.last_name, worker->last_name) != 0) {
            printf("ERROR: mismatching 'last_name', %s != %s (loop '%d')\n", 
                   eotd.last_name, worker->last_name, i);
            printf("RACE CONDITION DETECTED: String corruption due to unsynchronized access\n");
            exit(0);
        }

        if (strcmp(eotd.department, worker->department) != 0) {
            printf("ERROR: mismatching 'department', %s != %s (loop '%d')\n", 
                   eotd.department, worker->department, i);
            printf("RACE CONDITION DETECTED: Data inconsistency found\n");
            exit(0);
        }

        if (eotd.room_number != worker->room_number) {
            printf("ERROR: mismatching 'room_number', %d != %d (loop '%d')\n", 
                   eotd.room_number, worker->room_number, i);
            printf("RACE CONDITION DETECTED: Numeric data corruption\n");
            exit(0);
        }
    }

    printf("SUCCESS: Employee data remained consistent (unlikely without mutex)\n");
    return 0;
}