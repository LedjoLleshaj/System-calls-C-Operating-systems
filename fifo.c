/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "err_exit.h"
#include "fifo.h"
#include "defines.h"

void make_fifo(char *path)
{
    if (mkfifo(path, S_IRUSR | S_IWUSR) == -1)
    {
        if (errno != EEXIST)
        {
            errExit("[fifo.c:make_fifo] mkfifo failed");
        }
    }
}

int openFifoRDONLY(char *path)
{
    int fifo_fd = -1;

    fifo_fd = open(path, O_RDONLY);
    if (fifo_fd == -1)
    {
        errExit("[fifo.c:create_fifo] open FIFO1 failed (read mode)");
    }
    else
        print_msg("Fifo opened succesfully in Read Only mode\n");

    return fifo_fd;

}

int openFifoWRONLY(char *path)
{
    int fifo_fd = -1;

    fifo_fd = open(path, O_WRONLY);
    if (fifo_fd == -1)
    {
        errExit("[fifo.c:create_fifo] open FIFO1 failed (write mode)");
    }
    else
        print_msg("Fifo opened succesfully in Write Only mode\n");

    return fifo_fd;

}
