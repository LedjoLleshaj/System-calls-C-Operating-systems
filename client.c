/**
 * @file client.c
 * @brief Contiene l'implementazione del client.
 *
 * @todo Spostare le funzioni non main fuori dal file client.c ? (ad esempio una opzione per la funzione dividi() e' metterla in files.c)
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/msg.h>

#include "defines.h"
#include "err_exit.h"
#include "signals.h"
#include "files.h"
#include "client.h"
#include "semaphore.h"
#include "shared_memory.h"
#include "fifo.h"

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

/// current working directory.
char CURRENT_DIRECTORY[BUFFER_SZ] = "";

/// contains the path that was passed as a parameter.
char *searchPath = NULL;

char *int_to_string(int value)
{
    // with NULL,0 sprintf give the neccessary number of char to convert the number
    int needed = snprintf(NULL, 0, "%d", value);

    // needed +1 for '\0'
    char *string = (char *)malloc(needed + 1);
    if (string == NULL)
    {
        print_msg("[client.c:int_to_string] malloc failed\n");
        exit(1);
    }

    snprintf(string, needed + 1, "%d", value);
    return string;
}

bool strEquals(char *string1, char *string2)
{
    return strcmp(string1, string2) == 0;
}

void SIGINTSignalHandler(int sig)
{
    // No need to do anything
    // https://man7.org/linux/man-pages/man7/signal-safety.7.html
    /**
     *
     *  When performing buffered I/O on a file, the stdio functions must
     *  maintain a statically allocated data buffer along with associated
     *  counters and indexes (or pointers) that record the amount of data
     *  and the current position in the buffer.  Suppose that the main
     *  program is in the middle of a call to a stdio function such as
     *  printf(3) where the buffer and associated variables have been
     *  partially updated.  If, at that moment, the program is
     *  interrupted by a signal handler that also calls printf(3), then
     *  the second call to printf(3) will operate on inconsistent data,
     *  with unpredictable results.
     *
     */
}

void operazioni_client0()
{

    // connect to the ipc
    if (shmid < 0)
        shmid = sharedMemoryGet(get_ipc_key(), MAX_MSG_PER_CHANNEL * sizeof(message_t));
    if (shm_message == NULL)
        shm_message = (message_t *)sharedMemoryAttach(shmid, S_IRUSR | S_IWUSR);
    //print_msg("Shared memory: allocated and connected\n");

    if (shm_flag_ID < 0)
        shm_flag_ID = sharedMemoryGet(get_ipc_key2(), MAX_MSG_PER_CHANNEL * sizeof(int));
    if (shm_flag == NULL)
        shm_flag = (int *)sharedMemoryAttach(shm_flag_ID, S_IRUSR | S_IWUSR);
    //print_msg("Shared memory flags: allocated and connected\n");

    if (semid < 0)
        semid = semGetID(get_ipc_key(), 6);
    //print_msg("Semaphores: obtained the id of the set of semaphores\n");

    if (fifo1_fd < 0)
        fifo1_fd = create_fifo(FIFO1_PATH, 'w');
    //print_msg("Conneted to FIFO 1\n");

    if (fifo2_fd < 0)
        fifo2_fd = create_fifo(FIFO2_PATH, 'w');
    //print_msg("Conneted to FIFO 2\n"); // collegamento a fifo2

    if (msqid < 0)
        msqid = msgget(get_ipc_key(), IPC_CREAT | S_IRUSR | S_IWUSR); // creo la coda dei messaggi
    //print_msg("Conneted to Message Queue\n");

    if (semidFifo < 0)
        semidFifo = semGetID(get_ipc_key2(), 4);
    //print_msg("Semaphores: obtained the id of the set of semaphores for fifos\n");

    // sets up the current working directory to a path passed as a parameter at the start of the program
    if (chdir(searchPath) == -1)
    {
        errExit("chdir failed");
    }

    // greets the user printing a message on the screen
    char *USER = getenv("USER");
    if (USER == NULL)
        USER = "unknown";

    char CURRDIR[BUFFER_SZ];
    if (getcwd(CURRDIR, sizeof(CURRDIR)) == NULL)
    {
        errExit("getcwd");
    }

    char *buffer = (char *)malloc((strlen(CURRDIR) + strlen(USER) + 50) * sizeof(char));
    memset(buffer, 0, (strlen(CURRDIR) + strlen(USER) + 50) * sizeof(char));
    strcat(buffer, "Ciao ");
    strcat(buffer, USER);
    strcat(buffer, ", ora inizio l'invio dei file contenuti in ");
    strcat(buffer, CURRDIR);
    strcat(buffer, "\n");
    print_msg(buffer);
    free(buffer);

    // search in CURRDIR and in subdirectories all the files that start with "sendme_"
    //  that have the dimension less than 4KByte
    files_list *sendme_files = NULL;
    sendme_files = find_sendme_files(searchPath, sendme_files);
    // print_msg("Found the following files:\n");
    // print_list(sendme_files);

    // count the number of files to send
    int filenr = count_files(sendme_files);
    printf("ci sono %d file 'sendme_'\n", filenr);

    char *n_string = int_to_string(filenr);
    message_t n_msg = {.mtype = FILE_NR_MTYPE, .sender_pid = getpid()};
    strcpy(n_msg.msg_body, n_string);
    // printf("msg body '%s' \n", n_msg.msg_body);
    // printf("n string '%s' \n", n_string);
    free(n_string); // free the string that contained the number of files to send

    // send the number of files to send on fifo1
    if (write(fifo1_fd, &n_msg, sizeof(n_msg)) == -1)
        errExit("write FIFO 1 failed");

    // print_msg("Sent the number of files by FIFO1\n");

    // printf("Received the IPC key : %x\n", get_ipc_key());

    // Wait for confirmation from the server
    bool n_received = false;
    while (!n_received)
    {

        semWait(semid, 0);
        // start critical section
        if (shm_message[0].mtype == FILE_NR_MTYPE)
        {
            if (strEquals(shm_message[0].msg_body, "OK"))
            {
                n_received = true;
            }
        }
        // end critical section
        semSignal(semid, 0);
    }

    print_msg("Server confirmed of receiving the information\n");

    // make the fifos non blocking
    // print_msg("making the fifos non blocking\n");
    semWait(semidFifo, 0);
    semWaitZero(semidFifo, 0);
    blockFD(fifo1_fd, 0);
    blockFD(fifo2_fd, 0);
    semWait(semidFifo, 1);
    semWaitZero(semidFifo, 1);
    // print_msg("made the fifos non blocking\n");

    // generate nr_files*processes for each file
    files_list *sendme_file = sendme_files;
    while (sendme_file != NULL)
    {
        // printf("Creating child and assigning them the file: %s\n", sendme_file->path);
        pid_t pid = fork();
        if (pid == -1)
        {
            errExit("fork failed");
        }
        else if (pid == 0)
        {
            // Copied the path to a new variable to free the child list
            char *path = malloc(strlen(sendme_file->path) + 1);
            if (path == NULL)
            {
                print_msg("[client.c:SIGINTSignalHandler] malloc failed\n");
                exit(1);
            }
            // printf("Done malloc (child %d)\n", getpid());
            strcpy(path, sendme_file->path);
            // printf("Copied the path that i have to elaborate (child %d)\n", getpid());

            // free the list of files to send
            free_list(sendme_files);
            // printf("Freed the memory of the list of files (child %d)\n", getpid());

            // execute the child process
            operazioni_figlio(path);
            // printf("Terminated the process (child %d)\n", getpid());

            // free the path of the file that was elaborated and terminate
            free(path);
            // printf("Freed the path that i have to elaborate (child %d)\n", getpid());

            exit(0);
        }

        // code executed by client_0 (the children terminate with exit(0)
        sendme_file = sendme_file->next;
    }

    // it puts on hold on the MsgQueue of a message from the server that it
    // informs that all output files were created by the server itself and that the server has finished its operations.
    print_msg("...waiting the finish message from server...\n");
    message_t end_msg;
    msgrcv(msqid, &end_msg, sizeof(struct message_t) - sizeof(long), DONE, 0); // lettura bloccante
    printf("Received the finish msg line: '%s'\n", end_msg.msg_body);

    // wait for the children to terminate
    while (wait(NULL) > 0)
        ;

    // free the list of files
    free_list(sendme_files);

    // make the fifos blocking again
    // print_msg("make the fifos blocking again\n");
    semWait(semidFifo, 2);
    semWaitZero(semidFifo, 2);
    blockFD(fifo1_fd, 1);
    blockFD(fifo2_fd, 1);
    // wait the server too
    semWait(semidFifo, 3);
    semWaitZero(semidFifo, 3);
    // print_msg("made the fifos blocking\n");
}

void SIGUSR1SignalHandler(int sig)
{
    // close everything
    //
    // NOTE: it is not possible to close without deleting the semaphores
    // and the message queue. That will be the server's job
    print_msg("Client is dead\n");
    if (shm_message != NULL)
        sharedMemoryDetach(shm_message);

    if (shm_flag != NULL)
        sharedMemoryDetach(shm_flag);

    if (fifo1_fd != -1)
    {
        if (close(fifo1_fd) == -1)
        {
            errExit("[client.c:SIGUSR1SignalHandler] close FIFO1 failed");
        }
    }

    if (fifo2_fd != -1)
    {
        if (close(fifo2_fd) == -1)
        {
            errExit("[client.c:SIGUSR1SignalHandler] close FIFO2 failed");
        }
    }

    // terminate
    exit(0);
}

void dividi(int fd, char *buf, size_t count, char *filePath, int parte)
{
    ssize_t bR = read(fd, buf, count);
    if (bR > 0)
    {
        // add the character '\0' to let printf know where a string ends
        buf[bR] = '\0';
        // printf("Parte %d file %s: '%s'\n",parte,filePath, buf);
    }
    else
    {
        printf("Could't read the file %d\n", parte);
    }
}

void operazioni_figlio(char *filePath)
{
    // printf("Child: %d working on file: %s\n", getpid(), filePath);

    // open the file
    int fd = open(filePath, O_RDONLY);

    // check if the file is open
    if (fd == -1)
    {
        errExit("open failed");
    }

    // get the number of charrs in the file
    long numChar;
    numChar = lseek(fd, 0, SEEK_END);

    // printf("The file %s contains %ld chars (dimension %ld)\n", filePath, numChar, getFileSize(filePath));

    // divied the file in four parts containing the same number of characters (the last file can have less characters)
    long msg_lengths[MSG_PARTS_NUM];

    // divide the file in 4 EQUAL parts
    for (int i = 0; i < MSG_PARTS_NUM; i++)
    {
        msg_lengths[i] = numChar / MSG_PARTS_NUM;
    }

    // add the rest of the chars starting from first part by 1
    for (int i = 0; i < (numChar % MSG_PARTS_NUM); i++)
    {
        msg_lengths[i] += 1;
    }

    // printf("The file %s is divided in this way: %ld %ld %ld %ld\n", filePath, msg_lengths[0], msg_lengths[1], msg_lengths[2], msg_lengths[3]);

    // set the file pointer to the beginning of the file
    lseek(fd, 0, SEEK_SET);

    // prepare the 4 messages (4 parts of the file content) to send

    char msg_buffer[MSG_PARTS_NUM][MSG_BUFFER_SZ + 1];

    for (int i = 0; i < MSG_PARTS_NUM; i++)
    {
        dividi(fd, msg_buffer[i], msg_lengths[i], filePath, i + 1);
    }

    // Wait on a semaphore until all clients have arrived at this point
    // Wait with semop () until it reaches zero.
    // printf("Child %d, Decrement the semaphore with the other children and wait\n", getpid());
    semWait(semid, 1);
    semWaitZero(semid, 1);
    // printf("Have arrived all the processes,starting to send (pid %d)\n", getpid());

    bool sent[MSG_PARTS_NUM] = {false};

    while (!arrayContainsAllTrue(sent, MSG_PARTS_NUM))
    {

        message_t supporto;
        memset(&supporto, 0, sizeof(supporto));

        if (sent[0] == false)
        {
            // send the message to FIFO1
            // send also the pid and the name of the file "sendme_"(with complete path)
            supporto.mtype = FIFO1_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path, filePath);
            strcpy(supporto.msg_body, msg_buffer[0]);

            // printf("Attempt to send the msg [ %s, %d, %s] on FIFO1\n",supporto.msg_body,supporto.sender_pid,supporto.file_path);

            if (semWait_NOWAIT(semid, 3) == 0)
            {
                if (write(fifo1_fd, &supporto, sizeof(supporto)) != -1)
                {
                    // written successfully
                    sent[0] = true;
                }
                else
                {
                    semSignal(semid, 3);
                }
            }
        }

        if (sent[1] == false)
        {
            // send the message to FIFO2
            // send also the pid and the name of the file "sendme_"(with complete path)
            supporto.mtype = FIFO2_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path, filePath);
            strcpy(supporto.msg_body, msg_buffer[1]);

            // printf("Attempt to send the msg [ %s, %d, %s] on FIFO2\n",supporto.msg_body,supporto.sender_pid,supporto.file_path);

            if (semWait_NOWAIT(semid, 4) == 0)
            {
                if (write(fifo2_fd, &supporto, sizeof(supporto)) != -1)
                {
                    // la scrittura ha avuto successo
                    sent[1] = true;
                }
                else
                {
                    semSignal(semid, 4);
                }
            }
        }

        if (sent[2] == false)
        {
            // send the message to Message Queue
            // send also the pid and the name of the file "sendme_"(with complete path))
            supporto.mtype = MSGQUEUE_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path, filePath);
            strcpy(supporto.msg_body, msg_buffer[2]);

            // printf("Attempt to send the msg [ %s, %d, %s] on msgQueue\n",supporto.msg_body,supporto.sender_pid,supporto.file_path);

            if (semWait_NOWAIT(semid, 5) == 0)
            {
                if (msgsnd(msqid, &supporto, sizeof(struct message_t) - sizeof(long), IPC_NOWAIT) != -1)
                {
                    // written successfully
                    sent[2] = true;
                }
                else
                {
                    semSignal(semid, 5);
                    // perror("msgsnd failed");
                }
            }
        }

        if (sent[3] == false)
        {
            // send the message to shared memory
            // send also the pid and the name of the file "sendme_"(with complete path)

            supporto.mtype = SHARED_MEMORY_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path, filePath);
            strcpy(supporto.msg_body, msg_buffer[3]);

            // printf("Attemp to enter shared memory (child %d)\n", getpid());
            if (semWait_NOWAIT(semid, 2) == 0)
            {
                // printf("Inside shared memory (child %d)\n", getpid());
                for (int i = 0; i < MAX_MSG_PER_CHANNEL; i++)
                {
                    if (shm_flag[i] == 0)
                    {
                        // printf("Found free position %d to send: %s\n", i, supporto.msg_body);
                        shm_flag[i] = 1;
                        sent[3] = true;
                        shm_message[i] = supporto;
                        break;
                    }
                }
                semSignal(semid, 2);
                // printf("Outside the shared memory (child %d)\n", getpid());
            }
        }
    }

    // printf("I'm child %d and i have finished sending the messages\n", getpid());

    // close the file
    if (close(fd) == -1)
    {
        errExit("close failed");
    }
}

int main(int argc, char *argv[])
{

    // memorize the current esecution directory
    if (getcwd(CURRENT_DIRECTORY, sizeof(CURRENT_DIRECTORY)) == NULL)
    {
        errExit("getcwd");
    }

    // check if the number of arguments is correct
    if (argc != 2)
    {
        print_msg("Please, execute this program passing the directory with the 'sendme_' files as a parameter\n");
        exit(1);
    }

    searchPath = argv[1];

    // create a mask that allows only SIGINT and SIGUSR1
    allowOnlySIGINT_SIGUSR1();

    // Set two signal handlers: one for SIGINT and one for SIGUSR1
    // to perform the main operations of Client 0 and to terminate the program
    if (signal(SIGINT, SIGINTSignalHandler) == SIG_ERR || signal(SIGUSR1, SIGUSR1SignalHandler) == SIG_ERR)
    {
        errExit("change signal handler failed");
    }

    // Suspend the process until a signal handler is executed.
    //  after the signal handler is executed, the process is suspended.
    //  the procces is terminated witha SIGINT
    while (true)
    {
        print_msg("\n");     
        print_msg("###########   Client Started succesfully   ############\n");
        print_msg("\n");
        pause();

        // blocks all signals (including SIGUSR1 and SIGINT) by modifying the mask
        blockAllSignals();
        //print_msg("Ho bloccato tutti i segnali\n");

        // execute the main operations of Client 0
        operazioni_client0();

        
        print_msg("\n");
        print_msg("###########   Client Finished succesfully   ############\n");
        print_msg("\n");

        // unblock the signals (SIGUSR1 and SIGINT) by modifying the mask
        allowOnlySIGINT_SIGUSR1();
    }

    return 0;
}