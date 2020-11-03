#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <stdbool.h>

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


typedef struct Item {
    char *filename;
    int lineNum;
    char *line;
    struct Item *next;
} item;

typedef struct Argument {
    struct dirent *entry;
    char *filePath;
    char *key;
    sem_t *threadMutex;
} arg;

// definition of the get_items function used to find the word in the files
item* get_items(char[], char[]);

int main( int argc, char *argv[]){
    if(argc != 3){
        printf("Invalid Command Line Arguments");
        return 0;
    }

    char *temp = argv[1];
    int requiredQueueSize = atoi(temp);
    int bufferSize = *argv[2];

    struct readyQueue *queue;
    sem_t *mutex, *count, *indicator;
    int sharedMemory;
    char str[MAX_DIR_PATH+MAX_KEYWORD+4];

    //exclusion semaphore
    if((mutex = sem_open(SEM_MUTEX, O_CREAT , 0666, 0)) == SEM_FAILED){
        printf("Error opening mutex %d\n", errno);
        return 0;
    }

    //open shared memory
    if((sharedMemory = shm_open(MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0660)) == -1){
        printf("Error opening shared memory %d\n", errno);
        return 0;
    }

    //set shared memory size
    if( ftruncate(sharedMemory, sizeof(struct readyQueue)) == -1){
        printf("Error truncating memory %d\n", errno);
        perror("Error printed by perror: ");
        return 0;
    }

    //map the memory
    if((queue = mmap(NULL, sizeof(struct readyQueue), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemory, 0 )) == MAP_FAILED){
        printf("Error mapping memory\n");
        return 0;
    }

    queue->startIndex = 0;
    queue->endIndex = 0;

    //counting semaphore used to indicate how many spots are available in the queue
    if((count = sem_open(SEM_COUNT, O_CREAT | O_EXCL, 0660, requiredQueueSize)) == SEM_FAILED){
        printf("Error opening count\n");
        return 0;
    }

    if((indicator = sem_open(SEM_INDICATOR, O_CREAT | O_EXCL, 0660, 0)) == SEM_FAILED){
        printf("Error opening indicator\n");
        return 0;
    }

    //release mutex once initialization is complete
    if(sem_post(mutex) == -1){
        printf("Error posting mutex\n");
        return 0;
    }

    bool notExit = true;
    char *ptr;

    while(notExit){
        if(sem_wait(indicator) == -1){
            printf("Error waiting for indicator\n");
            return 0;
        }

        strcpy(str, queue->keywords[queue->startIndex]);
        queue->startIndex = (queue->startIndex + 1) % requiredQueueSize;

        ptr = strstr(str, "exit");
        if(ptr != NULL){
            notExit = true;
        }
        if(sem_post(count) == -1){
            printf("Error posting count\n");
            return 0;
        }

        printf("The string passed was %s\n", str);
    }

    return 0;
}

// get_items finds when a string is found within a line and creates the respective items
// returning the first item
item* get_items(char filePath[MAX_DIR_PATH], char key[MAX_KEYWORD]) {
    item *initial_item = NULL;

    // file stuctures native to c
    DIR *dir;
    struct dirent *entry;
    struct stat stats;
    sem_t *writerMutex, *empty, *full;

    if((sem_init(writerMutex, 0, 0)) == -1){
        perror("Initiating writerMutex failed\n");
        return;
    }

    if((sem_init(empty, 0, 0)) == -1){
        perror("Initializing empty failed\n");
        return;
    }

    

     // checks if directory can open
    if ((dir = opendir(filePath)) == NULL)
        perror("could not open directory");


    else {
        
        // finds different entries in the directory
        while ((entry = readdir(dir)) != NULL){

            arg * new_arg;
            new_arg = (arg *) malloc(sizeof(arg));
            new_arg->entry = entry;
            new_arg->filePath = filePath;
            new_arg->key = key;
            new_arg->threadMutex = writerMutex;
            
            pthread_t tpid;
            pthread_create(&tpid, NULL, find_lines, &new_arg);

        }

        // here i need to create a new writer thread that writes the solutions to the file

        //here we would need to join the writer thread back to the function and end the process
    }

}

void *find_lines(void* argument) {
    arg *args = (struct dirent *)argument;

    char path[MAX_DIR_PATH];
    strcpy(path, args->filePath);
    strcat(path, "/");
    strcat(path, args->entry->d_name);

    FILE *fp;
    char line[1024];
    fp = fopen(path, "r+");

    int lineNum = 0;
    while (fgets(line, MAX_LINE_SIZE, (FILE*)fp) != EOF) {
        lineNum++;
        char *ptr = strstr(line, args->key);

        if (ptr != NULL) { // the string is found within the line
            // creation of the new item
            item *i = NULL;
            i = (item *) malloc(sizeof(item));
            i->filename = strdup(args->entry->d_name);
            i->lineNum = lineNum;
            i->line = strdup(line);


// somehow i need to add these items to a shared buffer, not sure how i do that tho

        }
    }
}