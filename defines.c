/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include <sys/ipc.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"


key_t get_ipc_key() {
    return get_project_ipc_key('b');
}


key_t get_ipc_key2() {
    return get_project_ipc_key('G');
}


key_t get_project_ipc_key(char proj_id) {
    key_t key = ftok(CURRENT_DIRECTORY, proj_id);

    if (key == -1)
        errExit("ftok failed");

    return key;
}


bool arrayContainsAllTrue(bool arr[], int len){
    for (int i = 0; i < len; i++){
        if (arr[i] == false){
            return false;
        }
    }
    return true;
}


int blockFD(int fd, int blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    return fcntl(fd, F_SETFL, flags) != -1;
}


void print_msg(char * msg){
    if (write(STDOUT_FILENO, msg, strlen(msg)) == -1){
        errExit("write stdout failed");
    }
}