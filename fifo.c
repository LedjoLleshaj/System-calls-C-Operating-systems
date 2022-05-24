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

void make_fifo(char * path) {
    if (mkfifo(path, S_IRUSR | S_IWUSR) == -1) {
        if (errno != EEXIST) {
            errExit("[fifo.c:make_fifo] mkfifo failed");
        }
    }
}


int create_fifo(char * path, char mode) {
    int fifo1_fd = -1;

    make_fifo(path);
    printf("Ho creato/ottenuto la FIFO\n");

    if (mode == 'r') {
        fifo1_fd = open(path, O_RDONLY);
        if (fifo1_fd == -1) {
            errExit("[fifo.c:create_fifo] open FIFO1 failed (read mode)");
        }
    }
    else if (mode == 'w') {
        fifo1_fd = open(path, O_WRONLY);
        if (fifo1_fd == -1) {
            errExit("[fifo.c:create_fifo] open FIFO1 failed (write mode)");
        }
    }
    else {
        print_msg("[fifo.c:create_fifo] mode should be 'r' or 'w'\n");
        exit(1);
    }

    return fifo1_fd;
}
