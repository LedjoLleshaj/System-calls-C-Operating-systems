/// @file err_exit.c
/// @brief This file contains the functions that are used to exit the program in case of errors.

#include "err_exit.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

void errExit(const char *msg)
{
    perror(msg);
    exit(1);
}