/// @file message_queue.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle CODE DEI MESSAGGI.

#include <sys/msg.h>
#include <sys/msg.h>
#include <errno.h>

#include "err_exit.h"
#include "message_queue.h"
#include "defines.h"


struct msqid_ds msqGetStats(int msqid){
    struct msqid_ds ds;

    if (msgctl(msqid, IPC_STAT, &ds) == -1)
        errExit("msgctl STAT");

    return ds;
}


void msqSetStats(int msqid, struct msqid_ds ds){

    if (msgctl(msqid, IPC_SET, &ds) == -1) {
        if(errno != EPERM) {
            errExit("msgctl SET");
        }
        else {
            print_msg("Couldn't set new config: not enough permissions. Continuing anyway\n");
        }
    }
}