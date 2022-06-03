/// @file defines.c
/// @brief Contains the implementations of definitions of the project.

#include <sys/ipc.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"


key_t get_ipc_key() {
    return get_project_ipc_key('A');
}


key_t get_ipc_key2() {
    return get_project_ipc_key('L');
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
    //https://www.gnu.org/software/libc/manual/html_node/Getting-File-Status-Flags.html
    int flags = fcntl(fd, F_GETFL, 0);
    /* If reading the flags failed, return error indication now. */
    if (flags == -1)
        return 0;
/* Set just the flag we want to set. */
    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;
/* Store modified flag word in the descriptor. */
    return fcntl(fd, F_SETFL, flags) != -1;
}


void print_msg(char * msg){
    if (write(STDOUT_FILENO, msg, strlen(msg)) == -1){
        errExit("write stdout failed");
    }
}