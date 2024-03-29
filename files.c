#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "strings.h"
#include "files.h"
#include "err_exit.h"

void print_list(files_list *head)
{
    files_list *current = head;

    printf("Files in the list:\n");
    int i = 0;
    while (current != NULL)
    {
        printf("%d. '%s'\n", i, current->path);
        current = current->next;
        i++;
    }
}

size_t append2Path(char *path, char *directory) {
    size_t lastPathEnd = strlen(path);
    // extends current seachPath: seachPath + / + directory
    strcat(strcat(&path[lastPathEnd], "/"), directory);
    return lastPathEnd;
}


bool StartsWith_EndsWith(const char *filename, const char *sendme,const char *out) {
     //if       filename contain "_sendme"                  &&     !contains _out return true
   return ((strncmp(filename, sendme, strlen(sendme)) == 0) && (strstr(filename, out) == NULL));
        
}

files_list *append(files_list *head, char *path)
{
    files_list *next = malloc(sizeof(files_list));

    next->next = NULL;

    // step 1. allocate memory to hold word
    next->path = malloc(strlen(path) + 1);

    // step 2. copy the current word
    strcpy(next->path, path);

    // printf("Adding %s that will become': %s\n", path, next->path);

    if (head == NULL)
    {
        return next;
    }

    files_list *current = head;
    while (current->next != NULL)
    {
        current = current->next;
    }

    current->next = next;
    return head;
}

void free_list(files_list *head)
{

    if (head != NULL)
    {
        files_list *current = head;
        files_list *next = NULL;

        while (current->next != NULL)
        {
            next = current->next;
            free(current->path);
            free(current);
            current = next;
        }
        free(current->path);
        free(current);
    }
}

int count_files(files_list *head)
{
    int num = 0;
    files_list *current = head;

    while (current != NULL)
    {
        num++;
        current = current->next;
    }

    return num;
}

long getFileSize(char *filePath)
{
    if (filePath == NULL)
        return -1;

    struct stat statbuf;
    if (stat(filePath, &statbuf) == -1)
        return -1;

    return statbuf.st_size;
}

int checkFileSize(char *filePath)
{
    if (getFileSize(filePath) <= 4096)
        return 1;
    return 0;
}

int checkFileName(char *fileName)
{
     //if filename contain "_sendme" && !contains _out return true
    return StartsWith_EndsWith(fileName, "sendme_" , "_out");
}


files_list *find_sendme_files(char *searchPath, files_list *head)
{
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
    while ((dentry = readdir(dirp)) != NULL)
    {
        // Skip . and ..
        if (strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0)
        {
            continue;
        }

        // is the current dentry a regular file?
        if (dentry->d_type == DT_REG)
        {
            // extend current searchPath with the file name
            size_t lastPath = append2Path(searchPath, dentry->d_name);

            // checking the file name
            int matchFileName = checkFileName(dentry->d_name);
            int matchSize = checkFileSize(searchPath);

            // if match is 1, then a research ...
            if (matchFileName == 1 && matchSize == 1)
            {
                // printf("Trovato nuovo file, lo aggiungo alla lista: %s\n", searchPath);
                head = append(head, searchPath);
            }

            // reset current searchPath
            searchPath[lastPath] = '\0';

            // is the current dentry a directory
        }
        else if (dentry->d_type == DT_DIR)
        {
            // extend current searchPath with the directory name
            size_t lastPath = append2Path(searchPath, dentry->d_name);
            // call search method
            head = find_sendme_files(searchPath, head);
            // reset current searchPath
            searchPath[lastPath] = '\0';
        }
        errno = 0;
    }

    if (errno != 0)
        errExit("readdir failed");

    if (closedir(dirp) == -1)
        errExit("closedir failed");

    return head;
}