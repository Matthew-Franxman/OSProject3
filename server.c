#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DIR_PATH 1024
#define MAX_KEYWORD 256
#define MAX_LINE_SIZE 1024
#define MAX_OUT_SIZE 2048
#define SIZE_OF_QUEUE 8

#define SEM_MUTEX "/sem_mutex"
#define SEM_COUNT "/sem_count"

#define MEMORY_NAME "/queue_of_keywords"

struct readyQueue{
    char *keywords[MAX_KEYWORD];
};
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

typedef struct Item {
    char *filename;
    int lineNum;
    char *line;
    struct Item *next;
} item;

// definition of the find_line function used to find the word in the files
item* find_line(char*, char*);

int main( int argc, char *argv[]){
    if(argc != 3){
        printf("Invalid Command Line Arguments");
        return 0;
    }

    int requiredQueueSize = *argv[1];
    int bufferSize = *argv[2];

    struct readyQueue *queue;
    sem_t *mutex, *count;
    int sharedMemory;

    //exclusion semaphore
    if((mutex = sem_open(SEM_MUTEX, O_CREAT , 0666, 0)) == SEM_FAILED){
            printf("Error opening mutex");
            return 0;
    }

    //open shared memory
    if((sharedMemory = shm_open(MEMORY_NAME, O_RDWR | O_CREAT |O_EXCL, 0660) == -1)){
        printf("Error opening shared memory");
        return 0;
    }

    //set shared memory size
    if( ftruncate(sharedMemory, sizeof(struct readyQueue)) == -1){
        printf("Error truncating memory");
        return 0;
    }

    //map the memory
    if((queue = mmap(NULL, sizeof(struct readyQueue), PROT_READ | PROT_WRITE, MAP_SHARED, sharedMemory, 0 )) == MAP_FAILED){
        printf("Error mapping memory");
        return 0;
    }

    //counting semaphore used to indicate how many spots are available in the queue
    if((count = sem_open(SEM_COUNT, O_CREAT | O_EXCL, 0660, requiredQueueSize)) == SEM_FAILED){
        printf("Error opening count");
        return 0;
    }

    


    return 0;
}

// find_line finds when a string is found within a line and creates the respective items
// returning the first item
item* find_line(char *filePath, char *key) {
    item* *initial_item = NULL;

    // file stuctures native to c
    DIR *dir;
    struct dirent *entry;
    struct stat stats;

     // checks if directory can open
    if ((dir = opendir(filePath)) == NULL)
        perror("could not open directory");


    else {
        item *last_item = NULL;
        
        // finds different entries in the directory
        while ((entry = readdir(dir)) != NULL){
            char path[1000];
            strcpy(path, filePath);
            strcat(path, "/");
            strcat(path, entry->d_name);

            FILE *fp;
            char line[1024];
            fp = fopen(path, "r+");

            int lineNum = 0;
            while (fgets(line, 1024, (FILE*)fp) != EOF) {
                lineNum++;
                char *ptr = strstr(line, key);

                if (ptr != NULL) { // the string is found within the line
                    // creation of the new item
                    item *i = NULL;
                    i = (item *) malloc(sizeof(item));
                    i->filename = strdup(entry->d_name);
                    i->lineNum = lineNum;
                    i->line = strdup(line);

                    // link it to the node in front
                    if (last_item != NULL) {
                        last_item->next = i;
                    }
                    else {
                        initial_item = i;
                    }
                    last_item = i;
                }
            }
        }
    }

    return initial_item;
}