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

typedef struct Buffer {
    item *head;
    item *tail;
    int size;
} buffer;

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
    buffer *b;
    sem_t *threadMutex;
    sem_t *threadFull;
    sem_t *threadEmpty;
    int *integ;
} arg;

// definition of the get_items function used to find the word in the files
item* get_items(char[], char[], int);

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
     item *initial_item = NULL;

    item buffer;
    int bufferSize;   if( ftruncate(sharedMemory, sizeof(struct readyQueue)) == -1){
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
    if(sem_post(mutex) == -1)
        perror("Error posting mutex\n");
    

    bool notExit = true;
    char *ptr;

    while(notExit){
        if(sem_wait(indicator) == -1)
            perror("Error waiting for indicator\n");

        strcpy(str, queue->keywords[queue->startIndex]);
        queue->startIndex = (queue->startIndex + 1) % requiredQueueSize;

        ptr = strstr(str, "exit");
        if(ptr != NULL){
            notExit = true;
        }
        if(sem_post(count) == -1)
            perror("Error posting count\n");
            
        

        printf("The string passed was %s\n", str);
    }

    if(sem_close(mutex) == -1)
        perror("error closing mutex");

    if(sem_close(count) == -1)
        perror("error closing count");

    if(sem_close(indicator) == -1)
        perror("error closing indicator");

    if(sem_unlink(SEM_MUTEX) == -1)
        perror("error unlinking mutex");

    if(sem_unlink(SEM_COUNT) == -1)
        perror("error unlinking count");

    if(sem_unlink(SEM_INDICATOR) == -1)
        perror("error unlinking indicator");

    if(shm_unlink(MEMORY_NAME) == -1)
        perror("error unlinking shared memory");

    
    return 0;
}

// get_items finds when a string is found within a line and creates the respective items
// returning the first item
item* get_items(char filePath[MAX_DIR_PATH], char key[MAX_KEYWORD], int bufferSize) {

    int *integ =0;

    buffer *b;
    b = (buffer *) malloc(sizeof(buffer));
    

    // file stuctures native to c
    DIR *dir;
    struct dirent *entry;
    struct stat stats;

    sem_t *writerMutex, *empty, *full;

    if((sem_init(writerMutex, 0, 0)) == SEM_FAILED){
        perror("Initiating writerMutex failed\n");
        return;
    }

    if((sem_init(empty, 0, 0)) == SEM_FAILED){
        perror("Initializing empty failed\n");
        return;
    }

    if((sem_init(full, 0, bufferSize)) == SEM_FAILED){
        perror("Initiating full failed\n");
    }

     // checks if directory can open
    if ((dir = opendir(filePath)) == NULL)
        perror("could not open directory");


    else {
        
        // finds different entries in the directory
        while ((entry = readdir(dir)) != NULL){

            //puts the path together
            char path[1000];
            strcpy(path, filePath);
            strcat(path, "/");
            strcat(path, entry->d_name);

            if (stat(path, &stats) == -1)
                continue;
            else {
                // if its a normal file
                if (S_ISREG (stats.st_mode)){
                    arg * new_arg;
                    new_arg = (arg *) malloc(sizeof(arg));
                    new_arg->entry = entry;
                    new_arg->filePath = filePath;
                    new_arg->key = key;
                    new_arg->b = b;
                    new_arg->threadMutex = writerMutex;
                    new_arg->threadFull = full;
                    new_arg->threadEmpty = empty;
                    new_arg->integ = integ;
                    integ++;

                    pthread_t tpid;
                    pthread_create(&tpid, NULL, find_lines, &new_arg);
                }
            }

        }
        arg * new_arg;
        new_arg = (arg *) malloc(sizeof(arg));
        new_arg->b = b;

        pthread_t wpid;
        pthread_create(&wpid, NULL, write_file, &new_arg);

        // TODO here we would need to join the writer thread back to the function and end the process
    }

    free(integ);
    free(b);
}

void *find_lines(void* argument) {
    arg *args = (struct arg *)argument;

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
            
            if(sem_wait(args->threadMutex) == SEM_FAILED){
                perror("Waiting on threadMutex failed\n");
            }

            if(sem_wait(args->threadFull) == SEM_FAILED){
                perror("Waiting on threadFull failed\n");
            }

            enqueue(args->b, i);

            if(sem_post(args->threadEmpty) == SEM_FAILED){
                perror("Posting threadEmpty failed\n");
            }

            if(sem_post(args->threadMutex) == SEM_FAILED){
                perror("Posting threadMutex failed\n");
            }

            
        }
    }
    args->integ--;
    free(args);
}

void *write_file(void* argument) {
    
    arg *args = (struct arg *)argument;

    // TODO if it is empty this bad boy needs to wait

    while(args->integ != 0 && dequeue(args->b) != NULL) {

        item *i = dequeue(args->b);

        char sentence[MAX_OUT_SIZE];

        FILE *outFile;

        outFile = fopen("output.txt", "w+");

        fprintf(outFile, "%s:%d:%s\n", i->filename, i->lineNum, i->line);

    }

    free(args);

}

// equeue adds an entry to the back of the buffer
void enqueue(buffer *b, item *i){
  if (isEmpty(b)){
    b->head = i;
    b->tail = i;
  }
  else {
    b->tail->next = i;
    b->tail = i;
  }
  b->size = b->size +1;
}

// dequeue pops the front item off of the buffer
// and makes the next item the head
item* dequeue(buffer *b) {

  if (isEmpty(b))
    return NULL;

  item *tmp;
  tmp = b->head;
  if (b->head != b->tail)
    b->head = b->head->next;
  b->size = b->size -1; 

  return tmp;
}

// isEmpty checks to see if the buffer is empty
int isEmpty(buffer *b){
  return (b->size == 0);
}