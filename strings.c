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


bool StartsWith(const char *a, const char *b) {
    if(strncmp(a, b, strlen(b)) == 0)  {
        return 1;
    }
    return 0;
}