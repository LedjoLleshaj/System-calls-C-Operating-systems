/// @file strings.c
/// @brief Contiene le funzioni specifiche per la gestione delle STRINGHE.

#include <stdbool.h>
#include <sys/types.h>
#include <string.h>

#include "strings.h"


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