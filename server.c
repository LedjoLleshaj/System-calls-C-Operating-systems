/**
 * @file server.c
 * @brief Contiene l'implementazione del server.
 *
 * @todo Spostare le funzioni non main fuori dal file server.c
 * @todo Creare una funzione che fa cio' che e' nel while(true)
 *
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

/// Percorso cartella eseguibile
char CURRENT_DIRECTORY[BUFFER_SZ] = "";

/// file descriptor della FIFO 1
int fifo1_fd = -1;
/// file descriptor della FIFO 2
int fifo2_fd = -1;
/// identifier della message queue
int msqid = -1;
/// identifier del set di semafori
int semid = -1;
/// identifier della memoria condivisa contenente i messaggi
int shmid = -1;
/// puntatore alla memoria condivisa contenente i messaggi
message_t *shm_message = NULL;
/// identifier della memoria condivisa contenente le flag cella libera/occupata
int shm_flag_ID = -1;
/// puntatore alla memoria condivisa contenente le flag cella libera/occupata
int *shm_flag = NULL;

/// e' una matrice che per ogni riga contiene le 4 parti di un file
message_t **matriceFile = NULL;

void SIGINTSignalHandler(int sig)
{

    // chiudi FIFO1
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

    // chiudi FIFO2
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

    // chiudi e dealloca memorie condivise
    if (shm_message != NULL)
        sharedMemoryDetach(shm_message);
    if (shmid != -1)
        sharedMemoryRemove(shmid);

    if (shm_flag != NULL)
        sharedMemoryDetach(shm_flag);
    if (shm_flag_ID != -1)
        sharedMemoryRemove(shm_flag_ID);

    // chiudi coda dei messaggi
    if (msqid != -1)
    {
        if (msgctl(msqid, IPC_RMID, NULL) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] msgctl failed");
        }
    }

    // chiudi semafori
    if (semid != -1)
        semDelete(semid);

    exit(0);
}

int string_to_int(char *string)
{

    return atoi(string);
}

void aggiungiAMatrice(message_t a, int filenr)
{
    bool added = false;
    //controlls if file is already there
    for (int i = 0; i < filenr && added == false; i++)
        for (int j = 0; j < 4 && added == false; j++)
        {
            if (strcmp(matriceFile[i][j].file_path, a.file_path) == 0)
            {
                matriceFile[i][a.mtype - 2] = a;
                added = true;
            }
        }
    //add the file on the first empty row
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
        // cerca righe complete
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
            // mancano dei pezzi di file, passa alla prossima riga
            continue;
        }

        // recupera path del file completo, aggiungi "_out" e aprilo
        char *temp = (char *)malloc((strlen(matriceFile[i][0].file_path) + 5) * sizeof(char)); // aggiungo lo spazio per _out
        if (temp == NULL)
        {
            print_msg("[server.c:main] malloc failed\n");
            exit(1);
        }
        strcpy(temp, matriceFile[i][0].file_path);
        strcat(temp, "_out"); // aggiungo _out
        
        //aprire il file in modalita scritura e anche O_TRUNC cosi se bisogna sovrascrivere un file
        //non diventa un casino
        int file = open(temp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

        if (file == -1)
        {
            errExit("open failed");
        }

        // prepara l'output e scrivilo sul file
        for (int j = 0; j < 4; j++)
        {
            char *stampa = costruisciStringa(matriceFile[i][j]);
            if (write(file, stampa, strlen(stampa) * sizeof(char)) == -1)
            {
                errExit("write output file failed");
            }

            if (write(file, "\n\n", 2) == -1)
            {
                errExit("write newline to output file failed");
            }
            free(stampa);
        }

        close(file);
        free(temp);

        // segna la riga come letta
        for (int j = 0; j < 4; j++)
        {
            matriceFile[i][j].mtype = EMPTY_MTYPE;
        }

        // se cerco file completi ad ogni arrivo di una parte di file,
        // trovero' sempre al massimo un file completo: smetti di scorrere la matrice
        break;
    }
}

char *costruisciStringa(message_t a)
{
    char buffer[20]; // serve per convertire il pid
    sprintf(buffer, "%d", a.sender_pid);

    char *stringa = (char *)malloc((strlen(a.msg_body) + strlen(a.file_path) + strlen(buffer) + 61) * sizeof(char));

    strcpy(stringa, "[Parte ");

    switch (a.mtype)
    {
    case FIFO1_PART:
        strcat(stringa, "1, del file ");
        break;

    case FIFO2_PART:
        strcat(stringa, "2, del file ");
        break;

    case MSGQUEUE_PART:
        strcat(stringa, "3, del file ");
        break;

    case SHARED_MEMORY_PART:
        strcat(stringa, "4, del file ");
        break;

    default:
        break;
    }

    strcat(stringa, a.file_path);
    strcat(stringa, " spedita dal processo ");
    strcat(stringa, buffer);
    strcat(stringa, " tramite ");

    switch (a.mtype)
    {
    case FIFO1_PART:
        strcat(stringa, "FIFO1]\n");
        break;
    case FIFO2_PART:
        strcat(stringa, "FIFO2]\n");
        break;
    case MSGQUEUE_PART:
        strcat(stringa, "MsgQueue]\n");
        break;
    case SHARED_MEMORY_PART:
        strcat(stringa, "ShdMem]\n");
        break;
    default:
        break;
    }

    strcat(stringa, a.msg_body);
    return stringa;
}

int main(int argc, char *argv[])
{

    // memorizza il percorso dell'eseguibile per ftok()
    if (getcwd(CURRENT_DIRECTORY, sizeof(CURRENT_DIRECTORY)) == NULL)
    {
        errExit("getcwd");
    }

    // imposta signal handler per gestire la chiusura dei canali di comunicazione

    if (signal(SIGINT, SIGINTSignalHandler) == SIG_ERR)
    {
        errExit("change signal handler failed");
    }

    print_msg("Ho impostato l'handler di segnali\n");

    // -- APERTURA CANALI DI COMUNICAZIONE

    // genera due FIFO (FIFO1 e FIFO2), una coda di messaggi (MsgQueue), un
    // segmento di memoria condivisa (ShdMem) ed un set di semafori per gestire la concorrenza su
    // alcuni di questi strumenti di comunicazione.

    printf("Recuperata la chiave IPC: %x\n", get_ipc_key());

    shmid = sharedMemoryGet(get_ipc_key(), MAX_MSG_PER_CHANNEL * sizeof(message_t));
    shm_message = (message_t *)sharedMemoryAttach(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
    print_msg("Memoria condivisa: allocata e connessa\n");

    shm_flag_ID = sharedMemoryGet(get_ipc_key2(), MAX_MSG_PER_CHANNEL * sizeof(int));
    shm_flag = (int *)sharedMemoryAttach(shm_flag_ID, S_IRUSR | S_IWUSR);
    print_msg("Memoria condivisa flag: allocata e connessa\n");

    semid = semGetCreate(get_ipc_key(), 10);
    short unsigned int semValues[10] = {1, 0, 0, 0, 0, 0, 1, 50, 50, 50};
    semSetAll(semid, semValues);
    print_msg("Semafori: creati e inizializzati\n");

    fifo1_fd = create_fifo(FIFO1_PATH, 'r');
    print_msg("Mi sono collegato alla FIFO 1\n"); // collegamento a fifo1

    fifo2_fd = create_fifo(FIFO2_PATH, 'r'); // collegamento a fifo2
    print_msg("Mi sono collegato alla FIFO 2\n");

    msqid = msgget(get_ipc_key(), IPC_CREAT | S_IRUSR | S_IWUSR); // collegamento alla coda di messaggi
    print_msg("Mi sono collegato alla coda dei messaggi\n");

    // limito la coda
    struct msqid_ds ds = msqGetStats(msqid);
    ds.msg_qbytes = sizeof(message_t) * MAX_MSG_PER_CHANNEL;
    msqSetStats(msqid, ds);

    while (true)
    {
        // Attendo il valore <n> dal Client_0 su FIFO1 e lo memorizzo
        message_t n_msg;
        if (read(fifo1_fd, &n_msg, sizeof(message_t)) == -1)
        {
            errExit("read failed");
        }

        printf("Il client mi ha inviato un messaggio che dice che ci sono '%s' file da ricevere\n", n_msg.msg_body);
        //numero di file
        int filenr = string_to_int(n_msg.msg_body);
        // inizializzo la matrice contenente i pezzi di file
        matriceFile = (message_t **)malloc(filenr * sizeof(message_t *));
        for (int i = 0; i < filenr; i++)
            matriceFile[i] = (message_t *)malloc(4 * sizeof(message_t));

        message_t empty;
        empty.mtype = EMPTY_MTYPE;
        // inizializzo i valori del percorso per evitare di fare confronti con null
        for (int i = 0; i < BUFFER_SZ + 1; i++)
            empty.file_path[i] = '\0';
        // riempio la matrice con una struttura che mi dice se le celle sono vuote
        for (int i = 0; i < filenr; i++)
            for (int j = 0; j < 4; j++)
                matriceFile[i][j] = empty;

        printf("Tradotto in numero e' %d (teoricamente lo stesso valore su terminale)\n", filenr);

        // inizializzazione semaforo dei figli
        for (int i = 0; i < 2; i++)
        {
            semSignal(semid, 1);
            semSignal(semid, 2);
            semSignal(semid, 3);
            semSignal(semid, 4);
        }

        /*
        Incrementato in runtime per valere tanto quanto sono il numero di file/figli.
        Aspetta che tutti i processi figlio di client_0 abbiano suddiviso il
        proprio file in 4 parti prima di mandarle sulle IPC o FIFO
        */
        for (int i = 0; i < filenr; i++)
            semSignal(semid, 5);

        // scrive un messaggio di conferma su ShdMem
        message_t received_msg = {.msg_body = "OK", .mtype = FILE_NR_MTYPE, .sender_pid = getpid()};

        semWait(semid, 0);
        // zona mutex
        shm_message[0] = received_msg;
        // fine zona mutex
        semSignal(semid, 0);
        print_msg("Ho mandato al client il messaggio di conferma.\n");

        // rendi fifo non bloccanti
        print_msg("Rendi fifo non bloccanti\n");
        semWait(semid, 1);
        semWaitZero(semid, 1);
        blockFD(fifo1_fd, 0);
        blockFD(fifo2_fd, 0);
        semWait(semid, 2);
        semWaitZero(semid, 2);
        print_msg("Rese fifo non bloccanti\n");

        // si mette in ricezione ciclicamente su ciascuno dei quattro canali.
        // una volta ricevute tutte e quattro le parti di un file le riunisce nell’ordine corretto e
        // salva le 4 parti in un file di testo in cui ognuna delle quattro parti e’ separata dalla successiva da una riga
        // bianca (carattere newline) ed ha l’intestazione
        // > Il file verrà chiamato con lo stesso nome e percorso del file originale ma con l'aggiunta del postfisso "_out"
        int arrivedParts = 0;
        //int n_tries = 0;
        while (arrivedParts < filenr * 4)
        {

            // memorizza il PID del processo mittente, il nome del file con percorso completo ed il pezzo
            // di file trasmesso
            message_t fifo1_message, fifo2_message, message_queue_part;

            // leggo da fifo1 la prima parte del file
            if (read(fifo1_fd, &fifo1_message, sizeof(fifo1_message)) != -1)
            {
                printf("[Parte1, del file %s spedita dal processo %d tramite FIFO1]\n%s\n", fifo1_message.file_path, fifo1_message.sender_pid, fifo1_message.msg_body);
                semSignal(semid, 7);
                aggiungiAMatrice(fifo1_message, filenr);
                findAndMakeFullFiles(filenr);
                arrivedParts++;
            }

            // leggo da fifo2 la seconda parte del file
            if (read(fifo2_fd, &fifo2_message, sizeof(fifo2_message)) != -1)
            {
                printf("[Parte2,del file %s spedita dal processo %d tramite FIFO2]\n%s\n", fifo2_message.file_path, fifo2_message.sender_pid, fifo2_message.msg_body);
                semSignal(semid, 8);
                aggiungiAMatrice(fifo2_message, filenr);
                findAndMakeFullFiles(filenr);
                arrivedParts++;
            }

            // leggo dalla coda di messaggi la terza parte del file
            if (msgrcv(msqid, &message_queue_part, sizeof(struct message_t) - sizeof(long), MSGQUEUE_PART, IPC_NOWAIT) != -1)
            {
                printf("[Parte3,del file %s spedita dal processo %d tramite MsgQueue]\n%s\n", message_queue_part.file_path, message_queue_part.sender_pid, message_queue_part.msg_body);
                semSignal(semid, 9);
                aggiungiAMatrice(message_queue_part, filenr);
                findAndMakeFullFiles(filenr);
                arrivedParts++;
            }

            // leggi dalla memoria condivisa
            print_msg("Tenta di entrare nella memoria condivisa\n");
            if (semWait_NOWAIT(semid, 6) == 0)
            {
                print_msg("Sono entrato nella memoria condivisa\n");
                for (int i = 0; i < MAX_MSG_PER_CHANNEL; i++)
                {
                    if (shm_flag[i] == 1)
                    {
                        printf("Trovata posizione da leggere %d, messaggio: '%s'\n", i, shm_message[i].msg_body);
                        shm_flag[i] = 0;
                        aggiungiAMatrice(shm_message[i], filenr);
                        findAndMakeFullFiles(filenr);
                        arrivedParts++;
                    }
                }
                print_msg("Tenta di uscire nella memoria condivisa\n");
                semSignal(semid, 6);
                print_msg("Sono uscito dalla memoria condivisa\n");
            }
                /*
            if (n_tries % 5000 == 0)
            {
                printf("Ancora un altro tentativo... Counter = %d\n", arrivedParts);
            }
            n_tries++;
            */
        }

        // quando ha ricevuto e salvato tutti i file invia un messaggio di terminazione sulla coda di
        // messaggi, in modo che possa essere riconosciuto da Client_0 come messaggio
        print_msg("Invio messaggio di fine al client\n");
        message_t end_msg = {.msg_body = "DONE", .mtype = DONE, .sender_pid = getpid()};
        msgsnd(msqid, &end_msg, sizeof(struct message_t) - sizeof(long), 0);
        print_msg("Inviato messaggio di fine al client\n");

        // rendi fifo bloccanti
        print_msg("Rendi fifo bloccanti\n");
        semWait(semid, 3);
        semWaitZero(semid, 3);
        //blockFD(fifo1_fd, 1);
        //blockFD(fifo2_fd, 1);
        semWait(semid, 4);
        semWaitZero(semid, 4);
        print_msg("Rese fifo bloccanti\n");

        // libera memoria della matrice buffer
        for (int i = 0; i < filenr; i++)
        {
            free(matriceFile[i]);
        }
        free(matriceFile);

        // si rimette in attesa su FIFO 1 di un nuovo valore n tornando all'inizio del ciclo
        print_msg("\n");
        print_msg("============FINISHED ELABORATING THE FILES================================\n");
        print_msg("\n");
    }

    return 0;
}