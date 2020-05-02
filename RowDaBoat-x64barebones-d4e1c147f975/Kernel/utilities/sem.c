#include <stdint.h>
#include <sem.h>
#include <scheduler.h>
#include <memoryManager.h>
#include <lib.h>
#include <screenDriver.h>
#include <genericQueue.h>

#define MAX_SEMAPHORE 100

typedef struct semaphore{
    uint16_t counter;
    char * name;
    genericQueue blockedProcessesPidQueue;
    uint16_t dependantProcessesCount;
    uint8_t active;
    uint8_t inUse;
}semaphore_t;

typedef struct semaphores{
    semaphore_t semArray[MAX_SEMAPHORE];
    uint16_t firstInactive;
    uint16_t size;
}semaphores_t;

static int isValidSem(uint16_t sem);
static void dumpSemaphore(semaphore_t sem);
static int32_t getSemIndexFromName(char * name);
static uint16_t initializeSem(char * name, uint16_t initValue);

static semaphores_t semaphores;

static uint8_t semCreationLock; //Only one Sem can be created or accessed at a time.

int32_t createSem(char * name, uint16_t initValue){
    if(name == NULL)
        return -1;

    enter_critical_region(&semCreationLock);

    int index = getSemIndexFromName(name);

    if(index != -1){
        semaphores.semArray[index].dependantProcessesCount++;
        leave_critical_region(&semCreationLock);
        return index;
    }

    if(semaphores.size >= MAX_SEMAPHORE){
        leave_critical_region(&semCreationLock);
        return -1;
    }

    index = initializeSem(name, initValue);    
    printString("Sem real code: "); printint(index); putchar('\n'); //Test

    leave_critical_region(&semCreationLock);

    return index;      
}

int semWait(uint16_t sem){
    if(!isValidSem(sem))
        return -1;

    enter_critical_region(&semaphores.semArray[sem].inUse);

    if(semaphores.semArray[sem].counter > 0){
        semaphores.semArray[sem].counter--;
        leave_critical_region(&semaphores.semArray[sem].inUse);
        return 0;
    }
    
    uint16_t runningProcessPID = getPID();

    if(enqueue(&semaphores.semArray[sem].blockedProcessesPidQueue, &runningProcessPID) == -1){
        leave_critical_region(&semaphores.semArray[sem].inUse);
        return -1;
    }

    block(runningProcessPID);

    leave_critical_region(&semaphores.semArray[sem].inUse);

    return 0;
}

int semPost(uint16_t sem){
    if(!isValidSem(sem))
        return -1;

    enter_critical_region(&semaphores.semArray[sem].inUse);

    if(!isQueueEmpty(&semaphores.semArray[sem].blockedProcessesPidQueue))
        unblock(*((uint16_t*)dequeue(&semaphores.semArray[sem].blockedProcessesPidQueue)));
    else
        semaphores.semArray[sem].counter++;
    
    leave_critical_region(&semaphores.semArray[sem].inUse);

    return 0;
}

void removeSem(uint16_t sem){
    if(!isValidSem(sem))
        return;

    enter_critical_region(&semaphores.semArray[sem].inUse);
    
    semaphores.semArray[sem].dependantProcessesCount--;

    if(semaphores.semArray[sem].dependantProcessesCount > 0){
        leave_critical_region(&semaphores.semArray[sem].inUse);
        return;
    }

    if(!isQueueEmpty(&semaphores.semArray[sem].blockedProcessesPidQueue))
        println("Error Semaphores - Closed Sem With Blocked Processes");

    semaphores.semArray[sem].active = 0;

    semaphores.size--;

    if(sem < semaphores.firstInactive)
        semaphores.firstInactive = sem;

    leave_critical_region(&semaphores.semArray[sem].inUse);
}

void dumpSem(){
    uint16_t activeCount = 0;
    printString("Total semaphores: "); printint(semaphores.size); putchar('\n');

    for(uint16_t i = 0; activeCount < semaphores.size; i++){
        if(semaphores.semArray[i].active){
            activeCount++;
            printString("Semaphore Number: "); printint(activeCount); putchar('\n');
            printString("Code: "); printint(i); putchar(' ');
            dumpSemaphore(semaphores.semArray[i]);
        }
    }
}

static int32_t getSemIndexFromName(char * name){
    uint16_t i = 0;

    for(uint16_t activeSemCount = 0; activeSemCount < semaphores.size; i++){
        if(semaphores.semArray[i].active){
            activeSemCount++;
            if(strcmp(name,semaphores.semArray[i].name))
                return i;
        }
    }
    return -1;
}

static uint16_t initializeSem(char * name, uint16_t initValue){
    uint16_t index = semaphores.firstInactive;

    semaphores.semArray[index].active = 1;
    semaphores.semArray[index].name = name;
    semaphores.semArray[index].counter = initValue;
    semaphores.semArray[index].dependantProcessesCount = 1;
    semaphores.semArray[index].inUse = 0;
    semaphores.semArray[index].blockedProcessesPidQueue.first = NULL; //Deberian ser innecesarios, pero por las dudas...
    semaphores.semArray[index].blockedProcessesPidQueue.last = NULL;
    
    semaphores.size++;

    for(uint16_t i = index + 1; i < MAX_SEMAPHORE; i++){
        if(!semaphores.semArray[i].active){
            semaphores.firstInactive = i;
            return index;
        }
    }
    semaphores.firstInactive = MAX_SEMAPHORE; //The array is full
    return index;
}

static int isValidSem(uint16_t sem){
    return sem < MAX_SEMAPHORE && semaphores.semArray[sem].active;
}

static void dumpSemaphore(semaphore_t sem){
    printString("Name: "); printString(sem.name);

    printString(" Counter: "); printint(sem.counter);

    printString(" Processes Dependant on Semaphore: "); printint(sem.dependantProcessesCount);

    (sem.active)? printString(" Is Active") : printString(" Is Not Active(PROBLEM)"); putchar('\n');

    if(!isQueueEmpty(&sem.blockedProcessesPidQueue)){
        println("Processes Blocked by Semaphore:");

        for(genericQueueNode * iter = sem.blockedProcessesPidQueue.first; iter != NULL; iter = iter->next){
            putchar('\t'); dumpProcessFromPID(*((uint16_t*)iter->data));
        }
    }
}