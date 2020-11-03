#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#define MAX_DIR_PATH 1024
#define MAX_KEYWORD 256
#define MAX_LINE_SIZE 1024
#define MAX_OUT_SIZE 2048
#define SIZE_OF_QUEUE 8

#define SEM_MUTEX "/sem_mutex"
#define SEM_COUNT "/sem_count"
#define SEM_INDICATOR "/sem_indicator"

#define MEMORY_NAME "/queue_of_keywords"

struct readyQueue{
    char keywords[1024][MAX_DIR_PATH+MAX_KEYWORD+4];
    int startIndex;
    int endIndex;
};


int main( int argc, char *argv[]){
    if(argc != 3){
        printf("Invalid Command Line Arguments\n");
        return 0;
    }

    char *temp = argv[1];
    int requiredQueueSize = atoi(temp);
    char* inputFileName = argv[2];

    struct readyQueue *queue;
    sem_t *mutex, *count, *indicator;
    int sharedMemory;

    if((mutex = sem_open(SEM_MUTEX, 0, 0, 0)) == SEM_FAILED){
        printf("Error opening mutex\n");
        return 0;
    }

    //open shared memory
    if((sharedMemory = shm_open(MEMORY_NAME, O_RDWR, 0)) == -1){
        printf("Error opening shared memory\n");
        return 0;
    }


    //map the memory
    if((queue = mmap(NULL, sizeof(struct readyQueue), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemory, 0 )) == MAP_FAILED){
        printf("Error mapping memory %d\n", errno);
        perror("Error by perror is ");
        return 0;
    }

    //counting semaphore used to indicate how many spots are available in the queue
    if((count = sem_open(SEM_COUNT, 0, 0, 0)) == SEM_FAILED){
        printf("Error opening count\n");
        return 0;
    }

    if((indicator = sem_open(SEM_INDICATOR, 0, 0, 0)) == SEM_FAILED){
        printf("Error opening indicator\n");
        return 0;
    }

    FILE *inputFile = fopen(inputFileName, "r");
    char dirName[MAX_DIR_PATH], word[MAX_KEYWORD];

    while(fscanf(inputFile, "%s", dirName) != EOF){
        if(sem_wait(count) == -1){
            printf("Error waiting on count\n");
            return 0;
        }

        if(sem_wait(mutex) == -1){
            printf("Error waiting on mutex\n");
            return 0;
        }

        if(strcmp(dirName, "exit") == 0){
            sprintf(queue->keywords[queue->endIndex], "%s", dirName);
            printf("Exit found\n");
        }
        else{
            fscanf(inputFile, "%s ", word);
            sprintf(queue->keywords[queue->endIndex], "%s %s", dirName, word);
            queue->endIndex = ((queue->endIndex) + 1) % requiredQueueSize;
            printf("Input is %s %s with index %d\n", dirName, word, queue->endIndex);
        }
        if(sem_post(mutex) == -1){
            printf("Error posting mutex\n");
            return 0;
        }

        if(sem_post(indicator) == -1){
            printf("Error posting indicator\n");
            return 0;
        }
    }
    if(munmap(queue, sizeof(struct readyQueue)) == -1){
            printf("Error unmapping memory\n");
            return 0;
        }

    return 0;
}
