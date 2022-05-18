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
char workingDirectory[BUFFER_SZ] = "";

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
msg_struct *shm_ptr = NULL;
/// identifier della memoria condivisa contenente le flag cella libera/occupata
int shm_check_id = -1;
/// puntatore alla memoria condivisa contenente le flag cella libera/occupata
int *shm_check_ptr = NULL;

/// e' una matrice che PER OGNI RIGA contiene le 4 parti di un file
msg_struct **matriceFile = NULL;

void SIGINTSignalHandler(int sig)
{

    // chiudi FIFO1
    if (fifo1_fd != -1)
    {
        if (close(fifo1_fd) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] close FIFO1 failed");
        }

        if (unlink(PATH_FIFO1) == -1)
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

        if (unlink(PATH_FIFO2) == -1)
        {
            errExit("[server.c:SIGINTSignalHandler] unlink FIFO2 failed");
        }
    }

    // chiudi e dealloca memorie condivise
    if (shm_ptr != NULL)
        shmdt_DettachMemory(shm_ptr);
    if (shmid != -1)
        shmCtl_RemoveShm(shmid);

    if (shm_check_ptr != NULL)
        shmdt_DettachMemory(shm_check_ptr);
    if (shm_check_id != -1)
        shmCtl_RemoveShm(shm_check_id);

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
        semCtlDelete(semid);

    exit(0);
}

int string_to_int(char *string)
{

    return atoi(string);
}

void aggiungiAMatrice(msg_struct MsgStruct,int righe){

    bool aggiunto=false;
    //cerco se ce gia nella matrice
    for(int i=0; i<righe && aggiunto==false; i++)
        for(int j=0; j<4 && aggiunto==false; j++){
            if(strcmp(matriceFile[i][j].filePath,MsgStruct.filePath)==0){
                //mtype-2 perche inizo le mtype delle diversi parti da 2
                //salvo nell pos mtype -2
                matriceFile[i][MsgStruct.mtype-2]=MsgStruct;
                aggiunto=true;
            }
        }
    //se non era nella matrice lo aggiungo
    for(int i=0; i<righe && aggiunto==false; i++){
        //cerco la prima riga vuota
        if(matriceFile[i][0].mtype==INIZIALIZZAZIONE_MTYPE && matriceFile[i][1].mtype==INIZIALIZZAZIONE_MTYPE && matriceFile[i][2].mtype==INIZIALIZZAZIONE_MTYPE && matriceFile[i][3].mtype==INIZIALIZZAZIONE_MTYPE){
            matriceFile[i][MsgStruct.mtype-2]=MsgStruct;
            aggiunto=true;
        }
    }
}


void findAndMakeFullFiles(int righe){
    for(int i=0; i<righe; i++){
        // cerca righe complete
        bool fullLine = true;
        for (int j=0; j < 4 && fullLine; j++) {
            if(matriceFile[i][j].mtype==INIZIALIZZAZIONE_MTYPE){
                fullLine = false;
            }
        }

        if (!fullLine){
            //non e completo ancora il messagio perche ci sono MTYPE = -1
            // mancano dei pezzi di file, passa alla prossima riga
            continue;
        }

        // recupera path del file completo, aggiungi "_out" e aprilo
        char *temp = (char *)malloc((strlen(matriceFile[i][0].filePath)+5)*sizeof(char)); // aggiungo lo spazio per "_out"
        if (temp == NULL){
            print_msg("[server.c:main] malloc failed\n");
            exit(1);
        }
        strcpy(temp, matriceFile[i][0].filePath);
        strcat(temp, "_out"); // aggiungo _out
        int file = open(temp, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

        if (file == -1) {
            errExit("open failed");
        }

        // prepara l'output e scrivilo sul file
        for(int j=0; j<4;j++){
            char * stampa = costruisciStringa(matriceFile[i][j]);
            if (write(file, stampa, strlen(stampa) * sizeof(char)) == -1){
                errExit("write output file failed");
            }

            if (write(file, "\n\n", 2) == -1){
                errExit("write newline to output file failed");
            }
            free(stampa);
        }

        close(file);
        free(temp);

        // segna la riga come letta
        for (int j=0; j < 4; j++) {
            matriceFile[i][j].mtype = INIZIALIZZAZIONE_MTYPE;
        }

        // se cerco file completi ad ogni arrivo di una parte di file,
        // trovero' sempre al massimo un file completo: smetti di scorrere la matrice
        break;
    }
}

char *costruisciStringa(msg_struct MsgStruct)
{
    char pid[20]; // serve per convertire il pid
    sprintf(pid, "%d", MsgStruct.sender_pid);

    char *stringa = (char *)malloc((strlen(MsgStruct.message) + strlen(MsgStruct.filePath) + strlen(pid) + 61) * sizeof(char));

    strcpy(stringa, "[Parte ");

    switch (MsgStruct.mtype)
    {
    case CONTAINS_FIFO1_FILE_PART:
        strcat(stringa, "1, del file ");
        break;

    case CONTAINS_FIFO2_FILE_PART:
        strcat(stringa, "2, del file ");
        break;

    case CONTAINS_MSGQUEUE_FILE_PART:
        strcat(stringa, "3, del file ");
        break;

    case CONTAINS_SHM_FILE_PART:
        strcat(stringa, "4, del file ");
        break;

    default:
        break;
    }

    strcat(stringa, MsgStruct.filePath);
    strcat(stringa, " spedita dal processo ");
    strcat(stringa, pid);
    strcat(stringa, " tramite ");

    switch (MsgStruct.mtype)
    {
    case CONTAINS_FIFO1_FILE_PART:
        strcat(stringa, "FIFO1]\n");
        break;
    case CONTAINS_FIFO2_FILE_PART:
        strcat(stringa, "FIFO2]\n");
        break;
    case CONTAINS_MSGQUEUE_FILE_PART:
        strcat(stringa, "MsgQueue]\n");
        break;
    case CONTAINS_SHM_FILE_PART:
        strcat(stringa, "ShdMem]\n");
        break;
    default:
        break;
    }

    strcat(stringa, MsgStruct.message);
    return stringa;
}

int main(int argc, char *argv[])
{

    // memorizza il percorso dell'eseguibile per ftok()
    if (getcwd(workingDirectory, sizeof(workingDirectory)) == NULL)
    {
        errExit("getcwd");
    }

    // imposta signal handler per gestire la chiusura dei canali di comunicazione

    if (signal(SIGINT, SIGINTSignalHandler) == SIG_ERR)
    {
        errExit("change signal handler failed");
    }

    printf("Ho impostato l'handler di segnali\n");

    // -- APERTURA CANALI DI COMUNICAZIONE

    // genera due FIFO (FIFO1 e FIFO2), una coda di messaggi (MsgQueue), un
    // segmento di memoria condivisa (ShdMem) ed un set di semafori per gestire la concorrenza su
    // alcuni di questi strumenti di comunicazione.

    printf("Recuperata la chiave IPC: %x\n", getIpcKey());

    shmid = shmGet_CreateShmid(getIpcKey(), MAX_MSG_PER_CHANNEL * sizeof(msg_struct));
    shm_ptr = (msg_struct *)shmat_AttachMemory(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
    printf("Memoria condivisa: allocata e connessa\n");
    //paralele
    shm_check_id = shmGet_CreateShmid(getIpcKey2(), MAX_MSG_PER_CHANNEL * sizeof(int));
    shm_check_ptr = (int *)shmat_AttachMemory(shm_check_id, S_IRUSR | S_IWUSR);
    printf("Memoria condivisa flag: allocata e connessa\n");

    semid = createSemid(getIpcKey(), 10);
    short unsigned int semValues[10] = {1, 0, 0, 0, 0, 0, 1, MAX_MSG_PER_CHANNEL, MAX_MSG_PER_CHANNEL, MAX_MSG_PER_CHANNEL};
    semCtlSetAll(semid, semValues);
    printf("Semafori: creati e inizializzati\n");

    fifo1_fd = createAndOpenFifo(PATH_FIFO1, 'r');
    printf("Mi sono collegato alla FIFO 1\n"); // collegamento a fifo1

    fifo2_fd = createAndOpenFifo(PATH_FIFO2, 'r'); // collegamento a fifo2
    printf("Mi sono collegato alla FIFO 2\n");

    msqid = msgget(getIpcKey(), IPC_CREAT | S_IRUSR | S_IWUSR); // collegamento alla message queue
    printf("Mi sono collegato alla coda dei messaggi\n");

    // limito la coda per contenere 50 messaggi
    struct msqid_ds ds = msgQueueGetStats(msqid);
    ds.msg_qbytes = sizeof(msg_struct) * MAX_MSG_PER_CHANNEL;
    msgQueueSetStats(msqid, ds);

    while (true)
    {
        // Attendo il valore <n> dal Client_0 su FIFO1 e lo memorizzo
        msg_struct n_msg;
        if (read(fifo1_fd, &n_msg, sizeof(msg_struct)) == -1)
        {
            errExit("read failed");
        }

        printf("Il client mi ha inviato un messaggio che dice che ci sono '%s' file da ricevere\n", n_msg.message);
        int n = string_to_int(n_msg.message);
        // inizializzo la matrice contenente i pezzi di file        
        matriceFile = (msg_struct **)malloc(n * sizeof(msg_struct *));

        for (int i = 0; i < n; i++)
            matriceFile[i] = (msg_struct *)malloc(4 * sizeof(msg_struct));

        msg_struct vuoto;
        vuoto.mtype = INIZIALIZZAZIONE_MTYPE;
        // inizializzo i valori del percorso per evitare di fare confronti con null
        //memset?
        for (int i = 0; i < BUFFER_SZ + 1; i++)
            vuoto.filePath[i] = '\0';

        // riempio la matrice con una struttura che mi dice se le celle sono vuote
        for (int i = 0; i < n; i++)
            for (int j = 0; j < 4; j++)
                matriceFile[i][j] = vuoto;

        printf("Tradotto in numero e' %d (teoricamente lo stesso valore su terminale)\n", n);

        // inizializzazione semaforo dei figli
        for (int i = 0; i < 2; i++)
        {
            semOpSignal(semid, 1);
            semOpSignal(semid, 2);
            semOpSignal(semid, 3);
            semOpSignal(semid, 4);
        }

        // semaforo per far attendere i figli
        for (int i = 0; i < n; i++)
            semOpSignal(semid, 5);

        // scrive un messaggio di conferma su ShdMem
        msg_struct received_msg = {.message = "OK", .mtype = CONTAINS_N, .sender_pid = getpid()};

        semOpWait(semid, 0);
        // zona mutex
        shm_ptr[0] = received_msg;
        // fine zona mutex
        semOpSignal(semid, 0);
        printf("Ho mandato al client il messaggio di conferma.\n");

        // rendi fifo non bloccanti
        printf("Rendi fifo non bloccanti\n");
        semOpWait(semid, 1);
        semOpWaitZero(semid, 1);
        blockFD(fifo1_fd, 0);
        blockFD(fifo2_fd, 0);
        semOpWait(semid, 2);
        semOpWaitZero(semid, 2);
        printf("Rese fifo non bloccanti\n");

        // si mette in ricezione ciclicamente su ciascuno dei quattro canali.
        // una volta ricevute tutte e quattro le parti di un file le riunisce nell’ordine corretto e
        // salva le 4 parti in un file di testo in cui ognuna delle quattro parti e’ separata dalla successiva da una riga
        // bianca (carattere newline) ed ha l’intestazione
        // > Il file verrà chiamato con lo stesso nome e percorso del file originale ma con l'aggiunta del postfisso "_out"
        int arrived_parts_counter = 0;
        int n_tries = 0;

        //n nr di file * 4 parti ciascun file
        while (arrived_parts_counter < n * 4)
        {

            // memorizza il PID del processo mittente, il nome del file con percorso completo ed il pezzo
            // di file trasmesso
            msg_struct supporto1, supporto2, supporto3;

            // leggo da fifo1 la prima parte del file
            if (read(fifo1_fd, &supporto1, sizeof(supporto1)) != -1)
            {
                printf("[Parte1, del file %s spedita dal processo %d tramite FIFO1]\n%s\n", supporto1.filePath, supporto1.sender_pid, supporto1.message);
                semOpSignal(semid, 7);
                aggiungiAMatrice(supporto1, n);
                findAndMakeFullFiles(n);
                arrived_parts_counter++;
            }

            // leggo da fifo2 la seconda parte del file
            if (read(fifo2_fd, &supporto2, sizeof(supporto2)) != -1)
            {
                printf("[Parte2,del file %s spedita dal processo %d tramite FIFO2]\n%s\n", supporto2.filePath, supporto2.sender_pid, supporto2.message);
                semOpSignal(semid, 8);
                aggiungiAMatrice(supporto2, n);
                findAndMakeFullFiles(n);
                arrived_parts_counter++;
            }

            // leggo dalla coda di messaggi la terza parte del file
            if (msgrcv(msqid, &supporto3, sizeof(struct msg_struct) - sizeof(long), CONTAINS_MSGQUEUE_FILE_PART, IPC_NOWAIT) != -1)
            {
                printf("[Parte3,del file %s spedita dal processo %d tramite MsgQueue]\n%s\n", supporto3.filePath, supporto3.sender_pid, supporto3.message);
                semOpSignal(semid, 9);
                aggiungiAMatrice(supporto3, n);
                findAndMakeFullFiles(n);
                arrived_parts_counter++;
            }

            // leggi dalla memoria condivisa
            printf("Tenta di entrare nella memoria condivisa\n");
            if (semOpNonBloccante(semid, 6) == 0)
            {
                printf("Sono entrato nella memoria condivisa\n");
                for (int i = 0; i < MAX_MSG_PER_CHANNEL; i++)
                {
                    if (shm_check_ptr[i] == 1)
                    {
                        printf("Trovata posizione da leggere %d, messaggio: '%s'\n", i, shm_ptr[i].message);
                        shm_check_ptr[i] = 0;
                        aggiungiAMatrice(shm_ptr[i], n);
                        findAndMakeFullFiles(n);
                        arrived_parts_counter++;
                    }
                }
                printf("Tenta di uscire nella memoria condivisa\n");
                semOpSignal(semid, 6);
                printf("Sono uscito dalla memoria condivisa\n");
            }
            
            if (n_tries % 5000 == 0)
            {
                printf("Ancora un altro tentativo... Counter =  %d\n", arrived_parts_counter);
            }
            n_tries++;
        
            
        }
        // quando ha ricevuto e salvato tutti i file invia un messaggio di terminazione sulla coda di
        // messaggi, in modo che possa essere riconosciuto da Client_0 come messaggio
        printf("Invio messaggio di fine al client\n");
        msg_struct end_msg = {.message= "DONE", .mtype = CONTAINS_DONE, .sender_pid = getpid()};
        msgsnd(msqid, &end_msg, sizeof(struct msg_struct) - sizeof(long), 0);
        printf("Inviato messaggio di fine al client\n");

        // rendi fifo bloccanti
        printf("Rendi fifo bloccanti\n");
        semOpWait(semid, 3);
        semOpWaitZero(semid, 3);
        blockFD(fifo1_fd, 1);
        blockFD(fifo2_fd, 1);
        semOpWait(semid, 4);
        semOpWaitZero(semid, 4);
        printf("Rese fifo bloccanti\n");

        // libera memoria della matrice buffer
        for (int i = 0; i < n; i++)
        {
            free(matriceFile[i]);
        }
        free(matriceFile);

        // si rimette in attesa su FIFO 1 di un nuovo valore n tornando all'inizio del ciclo
        printf("\n");
        printf("==========================================================\n");
        printf("\n");
    }

    return 0;
}