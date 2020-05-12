#include <stdint.h>
#include <usrlib.h>
#include <stddef.h>

#define MAX_PHIL 50
#define MIN_PHIL 2
#define SEM_NAME "_phyl"
#define NAME_LEN 20
#define EOF 27  // ESC

#define LEFT_CHOPSTICK(p) (chopstick[p])
#define RIGHT_CHOPSTICK(p) (chopstick[p + 1 % phyloCount])

enum state {HUNGRY, EATING, THINKING};


typedef struct phylo{
    uint64_t pid;
    enum state state;
}phylo_t;

static sem_t chopstick[MAX_PHIL];

phylo_t phylosophers[MAX_PHIL];
uint16_t phyloCount;

void phylo(){
    init();

    char c;
    while( (c = getChar()) != EOF){
        switch (c){
        case 'a':
            createPhylosopher();
            break;
        case 'r':
            removePhylosopher();
            break;
        }
    }
}

static int evenPhylo(int argc, char ** argv){
    
    uint16_t phyloID = atoi(argv[1]);
    if(phyloID % 2 != 0)
        return 1;

    while(1){
        phylosophers[phyloID].state = HUNGRY;
        takeLeftChop(phyloID);
        takeRightChop(phyloID);

        phylosophers[phyloID].state = EATING;
        sleep(1);

        releaseRightChop(phyloID);
        releaseLeftChop(phyloID);
        phylosophers[phyloID].state = THINKING;
        
        sleep(2);
    }
    return 0;
}

static int oddPhylo(int argc, char ** argv){

    uint16_t phyloID = atoi(argv[1]);
    if(phyloID % 2 == 0)
        return 1;
        
    while(1){
        phylosophers[phyloID].state = HUNGRY;
        takeRightChop(phyloID);
        takeLeftChop(phyloID);

        phylosophers[phyloID].state = EATING;
        sleep(1);

        releaseLeftChop(phyloID);
        releaseRightChop(phyloID);
        phylosophers[phyloID].state = THINKING;

        sleep(2);
    }
    return 0;
}   

static void createPhylosopher(){
    char phyloName[NAME_LEN];
    char phyloID[NAME_LEN];
    uint16_t phylo = phyloCount;

    uintToBase(phylo, phyloName, 10);
    uintToBase(phylo, phyloID, 10);
    strcat(phyloName, SEM_NAME);

    phylosophers[phylo].state = THINKING;
    
    guaranteeThinkingPhylo(phylo - 1);

    chopstick[phylo] = createSem(phyloName, 1);
    phyloCount++;

    unblock(phylosophers[phylo - 1].pid);

    // if(phylosophers[phyloCount-1].state != THINKING){
    //     semWait(chopstick[phylo]);
    //     semPost(chopstick[0]);
    // }

    char* argv[] = {phyloName, phyloID};
    phylosophers[phylo].pid = initializeProccess((phylo % 2 == 0)? evenPhylo : oddPhylo, 0, 2, argv, NULL);
}

static void removePhylosopher(){
   if(phyloCount <= MIN_PHIL)
        return;

    uint16_t phylo = phyloCount - 1;
    uint16_t phyloLeft = phylo - 1;
     
    guaranteeThinkingPhylo(phylo - 1);
    guaranteeThinkingPhylo(phyloLeft);

    kill(phylosophers[phylo].pid);

    phyloCount--;

    unblock(phylosophers[phyloLeft].state);

    removeSem(chopstick[phylo]);
}

static void init(){
    char phyloName[NAME_LEN];
    char phyloID[NAME_LEN];

    phyloCount = MIN_PHIL;

    for(uint16_t i = 0; 0 < MIN_PHIL; i++){
        uintToBase(i, phyloName, 10);
        strcat(phyloName, SEM_NAME);
        chopstick[i] = createSem(phyloName, 1);
    }

    for(uint16_t i = 0; 0 < MIN_PHIL; i++){
        uintToBase(i, phyloName, 10);
        strcat(phyloName, SEM_NAME);
        uintToBase(i, phyloID, 10);

        phylosophers[i].state = THINKING;

        char* argv[] = {phyloName, phyloID};
        phylosophers[i].pid = initializeProccess((i % 2 == 0)? evenPhylo : oddPhylo, 0, 2, argv, NULL)
    }
}

// Garantiza que el phylo especificado este bloqueado en el estado THINKING
static void guaranteeThinkingPhylo(uint16_t phylo){
    while(1){

        // Phylo ya esta bloqueado entonces seguro no esta THINKING
        if(block(phylosophers[phylo].pid) == 1)
            sleep(1);

        // Phylo esta usando los cubiertos
        else if(phylosophers[phylo].state != THINKING){
            unblock(phylosophers[phylo].pid);
            sleep(1);
        }   
        // Phylo esta pensando
        else
            return;
    }
}

static void takeRightChop(uint16_t phyloID){
    semWait(RIGHT_CHOPSTICK(phyloID));
}

static void takeLeftChop(uint16_t phyloID){
    semWait(LEFT_CHOPSTICK(phyloID));
}

static void releaseRightChop(uint16_t phyloID){
    semPost(RIGHT_CHOPSTICK(phyloID));
}

static void releaseLeftChop(uint16_t phyloID){
    semPost(LEFT_CHOPSTICK(phyloID));
}