#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "strings.h"
#include "files.h"
#include "err_exit.h"


void print_list(files_list * head) {
    files_list * current = head;

    printf("File nella lista:\n");
    int i = 0;
    while (current != NULL) {
        printf("%d. '%s'\n", i, current->path);
        current = current->next;
        i++;
    }
}


files_list * append(files_list * head, char * path) {
    files_list * next = malloc(sizeof(files_list));

    next->next = NULL;

    //step 1. allocate memory to hold word
    next->path = malloc(strlen(path)+1);

    //step 2. copy the current word
    strcpy(next->path, path);

    // printf("Sto aggiungendo %s che diventera': %s\n", path, next->path);

    if (head == NULL) {
        return next;
    }

    files_list * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = next;
    return head;
}


void free_list(files_list * head) {

    if (head != NULL) {
        files_list * current = head;
        files_list * next = NULL;

        while (current -> next != NULL) {
            next = current->next;
            free(current->path);
            free(current);
            current = next;
        }
        free(current->path);
        free(current);
    }
}


int count_files(files_list * head) {
    int num = 0;
    files_list * current = head;

    while (current != NULL) {
        num++;
        current = current->next;
    }

    return num;
}


long getFileSize(char * filePath) {
    if (filePath == NULL)
        return -1;

    struct stat statbuf;
    if (stat(filePath, &statbuf) == -1)
        return -1;

    return statbuf.st_size;
}


int checkFileSize(char * filePath) {
    if (getFileSize(filePath) <= 4096)
        return 1;
    return 0;
}

int checkFileName(char * fileName) {
    // 1 se il nome del file inizia con "sendme_", 0 altrimenti
    return StartsWith(fileName, "sendme_");
}


files_list * find_sendme_files(char *searchPath, files_list * head) {
    // open the current searchPath
    DIR *dirp = opendir(searchPath);
    if (dirp == NULL)
        return head;

    // readdir returns NULL when end-of-directory is reached.
    // In oder to get when an error occurres, we set errno to zero, and the we
    // call readdir. If readdir returns NULL, and errno is different from zero,
    // an error must have occurred.
    errno = 0;
    // iter. until NULL is returned as a result
    struct dirent *dentry;
    while ( (dentry = readdir(dirp)) != NULL) {
        // Skip . and ..
        if (strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0) {
            continue;
        }

        // is the current dentry a regular file?
        if (dentry->d_type == DT_REG) {
            // extend current searchPath with the file name
            size_t lastPath = append2Path(searchPath, dentry->d_name);

            // checking the file name
            int matchFileName = checkFileName(dentry->d_name);
            int matchSize = checkFileSize(searchPath);

            // if match is 1, then a research ...
            if (matchFileName == 1 && matchSize == 1) {
                // printf("Trovato nuovo file, lo aggiungo alla lista: %s\n", searchPath);
                head = append(head, searchPath);
            }

            // reset current searchPath
            searchPath[lastPath] = '\0';

        // is the current dentry a directory
        }
        else if (dentry->d_type == DT_DIR) {
            // exetend current searchPath with the directory name
            size_t lastPath = append2Path(searchPath, dentry->d_name);
            // call search method
            head = find_sendme_files(searchPath, head);
            // reset current searchPath
            searchPath[lastPath] = '\0';
        }
        errno = 0;
    }

    if (errno != 0)
        ErrExit("readdir failed");

    if (closedir(dirp) == -1)
        ErrExit("closedir failed");

    return head;
}