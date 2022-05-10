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


bool StartsWith(const char *string1, const char *string2) {
    if(strncmp(string1, string2, strlen(string2)) == 0)  {
        return 1;
    }
    return 0;
}