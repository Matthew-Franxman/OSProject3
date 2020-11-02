#include <stdio.h>
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