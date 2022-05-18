/// @file message_queue.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle CODE DEI MESSAGGI.

#include <sys/msg.h>
#include <errno.h>

#include "err_exit.h"
#include "message_queue.h"

struct msqid_ds msgQueueGetStats(int msqid){
    struct msqid_ds ds;

    if (msgctl(msqid, IPC_STAT, &ds) == -1)
        errExit("msgctl STAT");

    return ds;
}


void msgQueueSetStats(int msqid, struct msqid_ds ds){

    if (msgctl(msqid, IPC_SET, &ds) == -1) {
        if(errno != EPERM) {
            errExit("msgctl SET");
        }
        else {
            errExit("Message Queue IPC_SET failed\n");
        }
    }
}