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

/// file descriptor prima fifo
int fifo1_fd = -1;
/// file descriptor seconda fifo
int fifo2_fd = -1;
/// id coda dei messaggi
int msqid = -1;
/// id set di semafori
int semid = -1;
/// id memoria condivisa messaggi
int shmid = -1;
/// puntatore memoria condivisa messaggi
msg_t * shm_ptr = NULL;
/// id memoria condivisa flag lettura/scrittura messaggi
int shm_check_id = -1;
/// puntatore memoria condivisa flag lettura/scrittura messaggi
int * shm_check_ptr = NULL;

/// Percorso cartella eseguibile
char EXECUTABLE_DIR[BUFFER_SZ] = "";

/// contiene percorso passato come parametro
char * searchPath = NULL;


char * int_to_string(int value){
    //con NULL,0 sprintf da il numero di char neccessari per il convertire il numero
    int needed = snprintf(NULL, 0, "%d", value);

    //needed +1 per \0
    char * string = (char *) malloc(needed+1);
    if (string == NULL){
        print_msg("[client.c:int_to_string] malloc failed\n");
        exit(1);
    }

    snprintf(string, needed+1, "%d", value);
    return string;
}


bool strEquals(char *a, char *b){
    return strcmp(a, b) == 0;
}


void SIGINTSignalHandler(int sig) {
    // non fare niente: https://man7.org/linux/man-pages/man7/signal-safety.7.html
}


void operazioni_client0() {

    // Connettiti alle IPC e alle FIFO
    if (shmid < 0)
        shmid = alloc_shared_memory(get_ipc_key(), MAX_MSG_PER_CHANNEL * sizeof(msg_t));
    if (shm_ptr == NULL)
        shm_ptr = (msg_t *) get_shared_memory(shmid, S_IRUSR | S_IWUSR);
    printf("Memoria condivisa: allocata e connessa\n");

    if (shm_check_id < 0)
        shm_check_id = alloc_shared_memory(get_ipc_key2(), MAX_MSG_PER_CHANNEL * sizeof(int));
    if (shm_check_ptr == NULL)
        shm_check_ptr = (int *) get_shared_memory(shm_check_id, S_IRUSR | S_IWUSR);
    printf("Memoria condivisa flag: allocata e connessa\n");

    if (semid < 0)
        semid = getSemaphores(get_ipc_key(), 10);
    printf("Semafori: ottenuto il set di semafori\n");

    if (fifo1_fd < 0)
        fifo1_fd = create_fifo(FIFO1_PATH, 'w');
    printf("Mi sono collegato alla FIFO 1\n");

    if (fifo2_fd < 0)
        fifo2_fd = create_fifo(FIFO2_PATH, 'w');
    printf("Mi sono collegato alla FIFO 2\n");  // collegamento a fifo2

    if (msqid < 0)
        msqid = msgget(get_ipc_key(), IPC_CREAT | S_IRUSR | S_IWUSR);  // creo la coda dei messaggi
    printf("Mi sono collegato alla coda dei messaggi\n");

    // imposta la sua directory corrente ad un path passato da linea di comando all’avvio del programma
    if (chdir(searchPath) == -1) {
        ErrExit("chdir failed");
    }

    // saluta l'utente stampando un messaggio a video
    char *USER = getenv("USER");
    if (USER == NULL)
        USER = "unknown";

    char CURRDIR[BUFFER_SZ];
    if (getcwd(CURRDIR, sizeof(CURRDIR)) == NULL) {
        ErrExit("getcwd");
    }

    char *buffer = (char *)malloc((strlen(CURRDIR)+strlen(USER)+50)*sizeof(char));
    memset(buffer, 0, (strlen(CURRDIR)+strlen(USER)+50)*sizeof(char));
    strcat(buffer,"Ciao ");
    strcat(buffer,USER);
    strcat(buffer,", ora inizio l'invio dei file contenuti in ");
    strcat(buffer,CURRDIR);
    strcat(buffer,"\n");
    print_msg(buffer);
    free(buffer);

    // ricerca in CURRDIR e nelle sotto-directory tutti i file che iniziano con "sendme_"
    // e la dimensione e' inferiore a 4KByte e memorizzali
    files_list * sendme_files = NULL;
    sendme_files = find_sendme_files(searchPath, sendme_files);
    printf("trovati i seguenti file:\n");
    print_list(sendme_files);

    // determina il numero <n> di questi file
    int n = count_files(sendme_files);
    printf("ci sono %d file 'sendme_'\n", n);

    // invia il numero di file tramite FIFO1 al server
    char * n_string = int_to_string(n);
    msg_t n_msg = {.mtype = CONTAINS_N, .sender_pid = getpid()};
    strcpy(n_msg.msg_body, n_string);
    printf("msg body '%s' \n", n_msg.msg_body);
    printf("n string '%s' \n", n_string);
    free(n_string);  // libera memoria occupata da <n> in formato stringa

    //scrivo il numero delle file sendme che ho strovato e li mando al server tramite fifo1
    if (write(fifo1_fd, &n_msg, sizeof(n_msg)) == -1)
        ErrExit("write FIFO 1 failed");

    printf("Ho inviato al server tramite FIFO1 il numero di file\n");

    printf("Recuperata la chiave IPC: %x\n", get_ipc_key());

    // si mette in attesa di conferma dal server su ShrMem
    bool n_received = false;
    while (!n_received) {

        semWait(semid, 0);
        // zona mutex
        if (shm_ptr[0].mtype == CONTAINS_N) {
            if (strEquals(shm_ptr[0].msg_body, "OK")) {
                n_received = true;
            }
        }
        // fine zona mutex
        semSignal(semid, 0);
    }

    printf("Il server ha confermato di aver ricevuto l'informazione\n");

    // rendi fifo non bloccanti
    printf("rendi fifo non bloccanti\n");
    semWait(semid, 1);
    semWaitZero(semid, 1);
    blockFD(fifo1_fd, 0);
    blockFD(fifo2_fd, 0);
    semWait(semid, 2);
    semWaitZero(semid, 2);
    printf("Rese fifo non bloccanti\n");

    // genera <n> processi figlio Client_i (uno per ogni file "sendme_")
    files_list * sendme_file = sendme_files;
    while (sendme_file != NULL) {
        printf("Creo figlio e gli ordino di gestire il file: %s\n", sendme_file->path);
        pid_t pid = fork();
        if (pid == -1) {
            ErrExit("fork failed");
        }
        else if (pid == 0) {
            // copio il percorso in una nuova variabile per liberare la lista del figlio
            char * path = malloc(strlen(sendme_file->path)+1);
            if (path == NULL){
                print_msg("[client.c:SIGINTSignalHandler] malloc failed\n");
                exit(1);
            }
            printf("Ho fatto la malloc (figlio %d)\n", getpid());
            strcpy(path, sendme_file->path);
            printf("Ho copiato il percorso del file che devo gestire (figlio %d)\n", getpid());

            // libero lista dei file del figlio
            free_list(sendme_files);
            printf("Ho liberato la memoria della lista dei file (figlio %d)\n", getpid());

            // esegui operazioni del figlio
            operazioni_figlio(path);
            printf("Ho terminato le operazioni principali (figlio %d)\n", getpid());

            // libera la memoria della variabile con il percorso e termina
            free(path);
            printf("Ho liberato la memoria occupata dal percorso del file che devo gestire (figlio %d)\n", getpid());

            exit(0);
        }

        // codice eseguito da client_0 (i figli terminano con l'exit(0))
        sendme_file = sendme_file->next;
    }

    // si mette in attesa sulla MsgQueue di un messaggio da parte del server che lo
    // informa che tutti i file di output sono stati creati dal server stesso e che il server ha concluso le sue operazioni.
    printf("Attendo di ricevere messaggio di fine.\n");
    msg_t end_msg;
    msgrcv(msqid, &end_msg, sizeof(struct msg_t)-sizeof(long), CONTAINS_DONE, 0);  // lettura bloccante
    printf("Ricevuto messaggio di fine: '%s'\n", end_msg.msg_body);

    // attendi fine dei processi figlio
    while(wait(NULL) > 0);

    // libera lista dei file
    free_list(sendme_files);

    // rendi fifo bloccanti
    printf("rendi fifo bloccanti\n");
    semWait(semid, 3);
    semWaitZero(semid, 3);
    blockFD(fifo1_fd, 1);
    blockFD(fifo2_fd, 1);
    semWait(semid, 4);
    semWaitZero(semid, 4);
    printf("rese fifo bloccanti\n");
}


void SIGUSR1SignalHandler(int sig) {
    // chiudi tutto
    //
    // NOTA: non e' possibile chiudere senza eliminare i semafori
    //       e la coda dei messaggi. Quello sara' il compito del server

    if (shm_ptr != NULL)
        free_shared_memory(shm_ptr);

    if (shm_check_ptr != NULL)
        free_shared_memory(shm_check_ptr);

    if (fifo1_fd != -1) {
        if (close(fifo1_fd) == -1){
            ErrExit("[client.c:SIGUSR1SignalHandler] close FIFO1 failed");
        }
    }

    if (fifo2_fd != -1) {
        if (close(fifo2_fd) == -1){
            ErrExit("[client.c:SIGUSR1SignalHandler] close FIFO2 failed");
        }
    }

    // termina
    exit(0);
}


void dividi(int fd, char *buf, size_t count, char *filePath, int parte) {
    ssize_t bR = read(fd, buf, count);
    if (bR > 0) {
        // add the character '\0' to let printf know where a string ends
        buf[bR] = '\0';
        printf("Parte %d file %s: '%s'\n",parte,filePath, buf);
    }
    else {
        printf("Non sono riuscito a leggere la parte %d\n",parte);
    }
}


void operazioni_figlio(char * filePath){
    printf("Sono il figlio %d e sto lavorando sul file %s\n", getpid(), filePath);

    // apre il file
    int fd = open(filePath, O_RDONLY);

    // controllo che non ci siano errori
    if (fd == -1) {
	    ErrExit("open failed");
    }

    // determina il numero di caratteri totali
    long numChar;
    numChar = lseek(fd, 0, SEEK_END);

    printf("Il file %s contiene %ld caratteri (dimensione %ld)\n", filePath, numChar, getFileSize(filePath));

    // divide il file in quattro parti contenenti lo stesso numero di caratteri (l'ultimo file puo' avere meno caratteri)
    long msg_lengths[MSG_PARTS_NUM];

    //divido il file in quatro parti tutti numeri uguali
    for (int i = 0; i < MSG_PARTS_NUM; i++) {
        msg_lengths[i] = numChar / MSG_PARTS_NUM;
    }

    //aggiungo il resto iniziando dalla prima parte del file
    for (int i = 0; i < (numChar % MSG_PARTS_NUM); i++) {
        msg_lengths[i] += 1;
    }

    printf("Il file %s contiene verra' diviso in parti con questi caratteri: %ld %ld %ld %ld\n", filePath, msg_lengths[0], msg_lengths[1], msg_lengths[2], msg_lengths[3]);

    //setto il puntatore al inizio del file
    lseek(fd, 0, SEEK_SET);
    // prepara i quattro messaggi (4 porzioni del contenuto del file) per l’invio

    char msg_buffer[MSG_PARTS_NUM][MSG_BUFFER_SZ + 1];

    for (int i = 0; i < MSG_PARTS_NUM; i++){
        dividi(fd, msg_buffer[i], msg_lengths[i], filePath, i+1);
    }

    // si blocca su un semaforo fino a quando tutti i client sono arrivati a questo punto
    // > Attesa con semop() finche' non arriva a zero.
    printf("Sono il figlio %d, decremento il semaforo di sincronizzazione con gli altri figli e attendo\n", getpid());
    semWait(semid, 5);
    semWaitZero(semid, 5);
    printf("Sono arrivati tutti i fratelli, parto a inviare il mio file (pid %d)\n", getpid());

    // -- INVIO 4 MESSAGGI
    // > NOTA: servono meccanismi di sincronizzazione tra i client. Il server gestira' il riordino dei messaggi.

    bool sent[MSG_PARTS_NUM] = {false};

    while (!arrayContainsAllTrue(sent, MSG_PARTS_NUM)) {

        msg_t supporto;
        memset(&supporto, 0, sizeof(supporto));

        if (sent[0] == false) {
            // invia il primo messaggio a FIFO1
            // > invia anche il proprio PID ed il nome del file "sendme_" (con percorso completo)
            supporto.mtype = CONTAINS_FIFO1_FILE_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path,filePath);
            strcpy(supporto.msg_body,msg_buffer[0]);

            printf("Tenta invio messaggio [ %s, %d, %s] su FIFO1\n",supporto.msg_body,supporto.sender_pid,supporto.file_path);

            if(semWaitNoBlocc(semid,7) == 0){
                if (write(fifo1_fd,&supporto,sizeof(supporto)) != -1){
                    // la scrittura ha avuto successo
                    sent[0] = true;
                }
                else{
                    semSignal(semid, 7);
                }
            }
        }

        if (sent[1] == false) {
            // invia il secondo messaggio a FIFO2
            // > invia anche il proprio PID ed il nome del file "sendme_" (con percorso completo)
            supporto.mtype = CONTAINS_FIFO2_FILE_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path,filePath);
            strcpy(supporto.msg_body,msg_buffer[1]);

            printf("Tenta invio messaggio [ %s, %d, %s] su FIFO2\n",supporto.msg_body,supporto.sender_pid,supporto.file_path);

            if(semWaitNoBlocc(semid,8) == 0){
                if (write(fifo2_fd,&supporto,sizeof(supporto)) != -1){
                    // la scrittura ha avuto successo
                    sent[1] = true;
                }
                else{
                    semSignal(semid, 8);
                }
            }
        }

        if (sent[2] == false) {
            // invia il terzo a MsgQueue (coda dei messaggi)
            // > invia anche il proprio PID ed il nome del file "sendme_" (con percorso completo)
            supporto.mtype = CONTAINS_MSGQUEUE_FILE_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path,filePath);
            strcpy(supporto.msg_body,msg_buffer[2]);

            printf("Tenta invio messaggio [ %s, %d, %s] su msgQueue\n",supporto.msg_body,supporto.sender_pid,supporto.file_path);

            if(semWaitNoBlocc(semid,9) == 0){
                if (msgsnd(msqid, &supporto, sizeof(struct msg_t)-sizeof(long), IPC_NOWAIT) != -1) {
                    // la scrittura ha avuto successo
                    sent[2] = true;
                }
                else{
                    semSignal(semid, 9);
                    perror("msgsnd failed");
                }
            }
        }

        if (sent[3] == false) {
            // invia il quarto a ShdMem (memoria condivisa)
            // > invia anche il proprio PID ed il nome del file "sendme_" (con percorso completo)

            supporto.mtype = CONTAINS_SHM_FILE_PART;
            supporto.sender_pid = getpid();
            strcpy(supporto.file_path,filePath);
            strcpy(supporto.msg_body,msg_buffer[3]);

            printf("Tento di entrare nella memoria condivisa (figlio %d)\n", getpid());
            if (semWaitNoBlocc(semid, 6) == 0){
                printf("Sono dentro la memoria condivisa (figlio %d)\n", getpid());
                for (int i = 0; i < MAX_MSG_PER_CHANNEL; i++) {
                    if (shm_check_ptr[i] == 0) {
                        printf("Trovata posizione libera %d per inviare: %s\n", i, supporto.msg_body);
                        shm_check_ptr[i] = 1;
                        sent[3] = true;
                        shm_ptr[i] = supporto;
                        break;
                    }
                }
                semSignal(semid, 6);
                printf("Sono fuori dalla memoria condivisa (figlio %d)\n", getpid());
            }
        }
    }

    printf("Sono il figlio %d e ho finito di inviare i messaggi\n", getpid());

    // chiude il file
    if (close(fd) == -1) {
        ErrExit("close failed");
    }

    // termina: gestito fuori dalla funzione
}


int main(int argc, char * argv[]) {

    // memorizza il percorso dell'eseguibile per ftok()
    if (getcwd(EXECUTABLE_DIR, sizeof(EXECUTABLE_DIR)) == NULL) {
        ErrExit("getcwd");
    }

    // assicurati che sia stato passato un percorso come parametro e memorizzalo
    if (argc != 2) {
        print_msg("Please, execute this program passing the directory with the 'sendme_' files as a parameter\n");
        exit(1);
    }

    searchPath = argv[1];

    // crea una maschera che gli consente di ricevere solo i segnali SIGINT e SIGUSR1
    block_sig_no_SIGINT_SIGUSR1();

    // imposta due signal handler: uno per SIGINT per eseguire le operazioni
    // principali di Client 0 e uno per SIGUSR1 per terminare il programma
    if (signal(SIGINT, SIGINTSignalHandler) == SIG_ERR || signal(SIGUSR1, SIGUSR1SignalHandler) == SIG_ERR) {
        ErrExit("change signal handler failed");
    }

    // sospendi processo fino all'esecuzione di un signal handler.
    // dopo la gestione del segnale, torna in sospensione.
    // > Il processo terminera' con il segnale SIGUSR1
    while (true) {
        pause();

        // blocca tutti i segnali (compresi SIGUSR1 e SIGINT) modificando la maschera
        block_all_signals();
        printf("Ho bloccato tutti i segnali\n");

        // esegui le operazioni del client_0
        operazioni_client0();

        printf("\n");
        printf("==========================================================\n");
        printf("\n");

        // sblocca i segnali SIGINT e SIGUSR1
        block_sig_no_SIGINT_SIGUSR1();
    }

    return 0;
}