/**
 * @file server.c
 * @brief Contains the main function of the server.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include "message_queue.h"
#include "server.h"

/// Execution path of the server.
char CURRENT_DIRECTORY[BUFFER_SZ] = "";

/// file descriptor of the server's FIFO 1.
int fifo1_fd = -1;
/// file descriptor of the server's FIFO 2.
int fifo2_fd = -1;
/// Message queue identifier.
int msqid = -1;
/// identifier of the semaphore.
int semid = -1;
/// identifier of the semaphore for the fifos.
int semidFifo = -1;
/// identifier of the shared memory.
int shmid = -1;
/// pointer to the shared memory that contains the messages.
message_t *shm_message = NULL;
/// identifier to the shared memory flags
int shm_flag_ID = -1;
/// pointer to the shared memory that contains the flags.
int *shm_flag = NULL;

/// two-dimensional array that for every line contains the four parts of the files.
message_t **matriceFile = NULL;

void SIGINTSignalHandler(int sig)
{
    print_msg("\n###########  Oh no you killed me!  #############\n\tClosing everything.\n");
    fflush(stdout);
    // close the fifos
    if (fifo1_fd != -1)
    {
        if (close(fifo1_fd) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] close FIFO1 failed");
        }

        if (unlink(FIFO1_PATH) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] unlink FIFO1 failed");
        }
    }

    // close the fifos
    if (fifo2_fd != -1)
    {
        if (close(fifo2_fd) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] close FIFO2 failed");
        }

        if (unlink(FIFO2_PATH) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] unlink FIFO2 failed");
        }
    }

    //  close and detach the shared memories
    if (shm_message != NULL)
        sharedMemoryDetach(shm_message);
    if (shmid != -1)
        sharedMemoryRemove(shmid);

    if (shm_flag != NULL)
        sharedMemoryDetach(shm_flag);
    if (shm_flag_ID != -1)
        sharedMemoryRemove(shm_flag_ID);

    // close the message queue
    if (msqid != -1)
    {
        if (msgctl(msqid, IPC_RMID, NULL) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] msgctl failed");
        }
    }

    // close the semaphores
    if (semid != -1)
        semDelete(semid);

    if (semidFifo != -1)
        semDelete(semidFifo);

    exit(0);
}

int string_to_int(char *string)
{
    /// @brief Converts a string to an integer.
    return atoi(string);
}

void addToMatrix(message_t a, int filenr)
{
    bool added = false;
    // controlls if file is already there
    for (int i = 0; i < filenr && added == false; i++)
        for (int j = 0; j < 4 && added == false; j++)
        {
            if (strcmp(matriceFile[i][j].file_path, a.file_path) == 0)
            {
                matriceFile[i][a.mtype - 2] = a;
                added = true;
            }
        }
    // add the file on the first empty row
    for (int i = 0; i < filenr && added == false; i++)
    {
        // find the first empty row
        if (matriceFile[i][0].mtype == EMPTY_MTYPE && matriceFile[i][1].mtype == EMPTY_MTYPE && matriceFile[i][2].mtype == EMPTY_MTYPE && matriceFile[i][3].mtype == EMPTY_MTYPE)
        {
            matriceFile[i][a.mtype - 2] = a;
            added = true;
        }
    }
}

void findAndMakeFullFiles(int righe)
{
    for (int i = 0; i < righe; i++)
    {
        // search for the first full row
        bool fullLine = true;
        for (int j = 0; j < 4 && fullLine; j++)
        {
            if (matriceFile[i][j].mtype == EMPTY_MTYPE)
            {
                fullLine = false;
            }
        }

        if (!fullLine)
        {
            // skip if the row is not full
            continue;
        }

        // take the path of the complete file and add "_out" then open it.
        char *temp = (char *)malloc((strlen(matriceFile[i][0].file_path) + 5) * sizeof(char));
        if (temp == NULL)
        {
            print_msg("[server.c:main] malloc failed\n");
            exit(1);
        }

        // remove comment if you want to see the "_out after the extension"
        /*
        //strcpy(temp, matriceFile[i][0].file_path);
        //strcat(temp, "_out"); // aggiungo _out
        */
        memset(temp, 0, (strlen(matriceFile[i][0].file_path) + 5) * sizeof(char));
        strncpy(temp, matriceFile[i][0].file_path, strlen(matriceFile[i][0].file_path) - 4);
        strcat(temp, "_out.txt"); // add "_out" to the end of the path

        // open the file in write mode and also O_TRUNC so if we need to overwrite a file
        int file = open(temp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        if (file == -1)
        {
            errExit("open failed");
        }

        // prepare the message and write it to the file
        for (int j = 0; j < 4; j++)
        {
            char *preparedMsg = prepareString(matriceFile[i][j]);
            if (write(file, preparedMsg, strlen(preparedMsg) * sizeof(char)) == -1)
            {
                errExit("write output file failed");
            }

            if (write(file, "\n\n", 2) == -1)
            {
                errExit("write newline to output file failed");
            }
            free(preparedMsg);
        }

        close(file);
        free(temp);

        // set the mtype of the row to EMPTY_MTYPE so we can reuse it.
        for (int j = 0; j < 4; j++)
        {
            matriceFile[i][j].mtype = EMPTY_MTYPE;
        }

        break;
    }
}

char *prepareString(message_t a)
{
    char buffer[20];
    sprintf(buffer, "%d", a.sender_pid);

    char *strprepare = (char *)malloc((strlen(a.msg_body) + strlen(a.file_path) + strlen(buffer) + 61) * sizeof(char));

    strcpy(strprepare, "[Parte ");

    switch (a.mtype)
    {
    case FIFO1_PART:
        strcat(strprepare, "1, del file ");
        break;

    case FIFO2_PART:
        strcat(strprepare, "2, del file ");
        break;

    case MSGQUEUE_PART:
        strcat(strprepare, "3, del file ");
        break;

    case SHARED_MEMORY_PART:
        strcat(strprepare, "4, del file ");
        break;

    default:
        break;
    }

    strcat(strprepare, a.file_path);
    strcat(strprepare, " spedita dal processo ");
    strcat(strprepare, buffer);
    strcat(strprepare, " tramite ");

    switch (a.mtype)
    {
    case FIFO1_PART:
        strcat(strprepare, "FIFO1]\n");
        break;
    case FIFO2_PART:
        strcat(strprepare, "FIFO2]\n");
        break;
    case MSGQUEUE_PART:
        strcat(strprepare, "MsgQueue]\n");
        break;
    case SHARED_MEMORY_PART:
        strcat(strprepare, "ShdMem]\n");
        break;
    default:
        break;
    }

    strcat(strprepare, a.msg_body);
    return strprepare;
}

int main(int argc, char *argv[])
{

    print_msg("\n");
    print_msg("###########   Server Started succesfully   ############\n");
    print_msg("\n");

    // memorize the path of the executable for ftok()
    if (getcwd(CURRENT_DIRECTORY, sizeof(CURRENT_DIRECTORY)) == NULL)
    {
        errExit("getcwd");
    }

    // setup signal handler for closing the communication channels
    if (signal(SIGINT, SIGINTSignalHandler) == SIG_ERR)
    {
        errExit("change signal handler failed");
    }

    // print_msg("Setup of Signal Handler done!\n");

    // printf("Obtained the IPC keys: %x\n", get_ipc_key());

    // setup the shared memory
    shmid = sharedMemoryGet(get_ipc_key(), MAX_MSG_PER_CHANNEL * sizeof(message_t));
    shm_message = (message_t *)sharedMemoryAttach(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
    // print_msg("Shared Memory:allocated and connected\n");

    shm_flag_ID = sharedMemoryGet(get_ipc_key2(), MAX_MSG_PER_CHANNEL * sizeof(int));
    shm_flag = (int *)sharedMemoryAttach(shm_flag_ID, S_IRUSR | S_IWUSR);
    // print_msg("Message Queue:allocated and connected\n");

    // setup the semaphores set
    semid = semGetCreate(get_ipc_key(), 10);
    short unsigned int semValues[6] = {1, 0, 1, 50, 50, 50};
    semSetAll(semid, semValues);
    // print_msg("Semaphores created and initialized\n");

    // setup fifos
    fifo1_fd = create_fifo(FIFO1_PATH, 'r');
    // print_msg("Connected to FIFO 1\n"); // collegamento a fifo1

    fifo2_fd = create_fifo(FIFO2_PATH, 'r'); // collegamento a fifo2
    // print_msg("Connected to FIFO 2\n");

    // setup message queue
    msqid = msgget(get_ipc_key(), IPC_CREAT | S_IRUSR | S_IWUSR); // collegamento alla coda di messaggi
    // print_msg("Connected to message queue\n");

    // setup semaphore set
    semidFifo = semGetCreate(get_ipc_key2(), 4);
    short unsigned int semFifoValues[4] = {0, 0, 0, 0};
    semSetAll(semidFifo, semFifoValues);
    // print_msg("Semaphores for fifos:created and initialized\n");

    // set the 50 limit to message queue
    struct msqid_ds ds = msqGetStats(msqid);
    ds.msg_qbytes = sizeof(message_t) * MAX_MSG_PER_CHANNEL;
    msqSetStats(msqid, ds);

    while (true)
    {
        //  Wait for a the file number to arrive from client.
        message_t n_msg;
        if (read(fifo1_fd, &n_msg, sizeof(message_t)) == -1)
        {
            errExit("read failed");
        }

        printf("The client sent me the file number '%s'.\n", n_msg.msg_body);

        // file number arrived, now we can start the communication
        int filenr = string_to_int(n_msg.msg_body);
        // printf("Tradotto in numero e' %d (teoricamente lo stesso valore su terminale)\n", filenr);
        //  initialize the two-dimensional array of messages
        matriceFile = (message_t **)malloc(filenr * sizeof(message_t *));
        for (int i = 0; i < filenr; i++)
            matriceFile[i] = (message_t *)malloc(4 * sizeof(message_t));

        message_t empty;
        empty.mtype = EMPTY_MTYPE;
        // initialize the two-dimensional array of messages to evitate confronting NULL pointers
        for (int i = 0; i < BUFFER_SZ + 1; i++)
            empty.file_path[i] = '\0';
        // refill the two-dimensional array of messages with empty messages
        for (int i = 0; i < filenr; i++)
            for (int j = 0; j < 4; j++)
                matriceFile[i][j] = empty;

        // prepare semaphore set for the communication
        short unsigned int setValFifoSem[4] = {2, 2, 2, 2};
        semSetAll(semidFifo, setValFifoSem);

        /*
             set it equal to the number of files/processes
             and wait for the other processes to finish their work
        */
        semSetVal(semid, 1, filenr);

        // write a confirmation message to the client
        message_t received_msg = {.msg_body = "OK", .mtype = FILE_NR_MTYPE, .sender_pid = getpid()};

        // trying to enter the critical section on shared memory
        semWait(semid, 0);
        shm_message[0] = received_msg;
        semSignal(semid, 0);
        print_msg("Sent to client the confirmation message.\n");

        // make fifo file descriptor nonblocking
        // print_msg("Rendi fifo non bloccanti\n");
        semWait(semidFifo, 0);
        semWaitZero(semidFifo, 0);
        blockFD(fifo1_fd, 0);
        blockFD(fifo2_fd, 0);
        semWait(semidFifo, 1);
        semWaitZero(semidFifo, 1);
        // print_msg("Rese fifo non bloccanti\n");

        // it cyclically receives on each of the four channels.
        // once all four parts of a file have been received, it reunites them in the correct order e
        // save the 4 parts in a text file in which each of the four parts is separated from the next by a newline
        // The file will be named with the same name and path as the original file but with the addition of the postfix "_out"
        int arrivedParts = 0;
        while (arrivedParts < filenr * 4)
        {

            // it stores the PID of the sending process, the name of the file with the complete path and the piece
            // of the file trasmited
            message_t fifo1_message, fifo2_message, message_queue_part;

            // read from fifo1 the first message part
            if (read(fifo1_fd, &fifo1_message, sizeof(fifo1_message)) != -1)
            {
                // printf("[Parte1, del file %s spedita dal processo %d tramite FIFO1]\n%s\n", fifo1_message.file_path, fifo1_message.sender_pid, fifo1_message.msg_body);
                semSignal(semid, 3);
                addToMatrix(fifo1_message, filenr);
                findAndMakeFullFiles(filenr);
                arrivedParts++;
            }

            // read from fifo2 the second message part
            if (read(fifo2_fd, &fifo2_message, sizeof(fifo2_message)) != -1)
            {
                // printf("[Parte2,del file %s spedita dal processo %d tramite FIFO2]\n%s\n", fifo2_message.file_path, fifo2_message.sender_pid, fifo2_message.msg_body);
                semSignal(semid, 4);
                addToMatrix(fifo2_message, filenr);
                findAndMakeFullFiles(filenr);
                arrivedParts++;
            }

            // read from the message queue the third message part
            if (msgrcv(msqid, &message_queue_part, sizeof(struct message_t) - sizeof(long), MSGQUEUE_PART, IPC_NOWAIT) != -1)
            {
                // printf("[Parte3,del file %s spedita dal processo %d tramite MsgQueue]\n%s\n", message_queue_part.file_path, message_queue_part.sender_pid, message_queue_part.msg_body);
                semSignal(semid, 5);
                addToMatrix(message_queue_part, filenr);
                findAndMakeFullFiles(filenr);
                arrivedParts++;
            }

            // read from the shared memory the fourth message part
            // print_msg("Attempting to enter shared memory\n");
            if (semWait_NOWAIT(semid, 2) == 0)
            {
                // print_msg("Inside shared memory\n");
                for (int i = 0; i < MAX_MSG_PER_CHANNEL; i++)
                {
                    if (shm_flag[i] == 1)
                    {
                        // printf("\nSHAREDMEMORY cycle %d,\n\nTrovata posizione da leggere %d, messaggio: '%s'\n\n\n", i, i, shm_message[i].msg_body);
                        shm_flag[i] = 0;
                        addToMatrix(shm_message[i], filenr);
                        findAndMakeFullFiles(filenr);
                        arrivedParts++;
                    }
                }
                // print_msg("Atempt to leave shared memory\n");
                semSignal(semid, 2);
                // print_msg("Outside of the shared memory\n");
            }
        }

        // when it has received and saved all the files it sends a termination message on the message queue
        // so that it can be recognized by Client_0 as a confirmation message
        print_msg("Sending Finish msg to Client\n");
        message_t end_msg = {.msg_body = "DONE", .mtype = DONE, .sender_pid = getpid()};
        msgsnd(msqid, &end_msg, sizeof(struct message_t) - sizeof(long), 0);
        // print_msg("Inviato messaggio di fine al client\n");

        // make fifo file descriptor blocking again
        // print_msg("Making fifo bloccking\n");
        semWait(semidFifo, 2);
        semWaitZero(semidFifo, 2);
        blockFD(fifo1_fd, 1);
        blockFD(fifo2_fd, 1);
        semWait(semidFifo, 3);
        semWaitZero(semidFifo, 3);
        // print_msg("Made fifo bloccking\n");

        // free allocated memory for the two-dimensional array storign the parts of the file
        for (int i = 0; i < filenr; i++)
        {
            free(matriceFile[i]);
        }
        free(matriceFile);

        // it waits on FIFO 1 for a new value n returning to the beginning of the cycle
        print_msg("\n");
        print_msg("###########   FINISHED ELABORATING THE FILES   ############\n");
        print_msg("\n");
    }

    return 0;
}