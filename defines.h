/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#define BUFFER_SZ 255

extern char workingDirectory[BUFFER_SZ];//ndrro emrin ne fund

/// Percorso file FIFO 1
#define PATH_FIFO1 "/home/lleshaj/Desktop/Sistemi-operativi/fifo1_file.txt"
/// Percorso file FIFO 2
#define PATH_FIFO2 "/home/lleshaj/Desktop/Sistemi-operativi/fifo2_file.txt"
/// mtype messaggio che contiene numero di file "sendme_"
#define CONTAINS_N 1
/// mtype messaggio che contiene prima parte del contenuto del file "sendme_"
#define CONTAINS_FIFO1_FILE_PART 2
/// mtype messaggio che contiene seconda parte del contenuto del file "sendme_"
#define CONTAINS_FIFO2_FILE_PART 3
/// mtype messaggio che contiene terza parte del contenuto del file "sendme_"
#define CONTAINS_MSGQUEUE_FILE_PART 4
/// mtype messaggio che contiene quarta parte del contenuto del file "sendme_"
#define CONTAINS_SHM_FILE_PART 5
/// mtype messaggio che contiene il messaggio di fine proveniente dal server
#define CONTAINS_DONE 6
/// mtype messaggio che contiene il valore usato per inizializzare la matrice contenente i file finali
#define INIZIALIZZAZIONE_MTYPE -1

/// numero massimo di messaggi per canale di comunicazione
#define MAX_MSG_PER_CHANNEL 50

// -- Macro suddivisione messaggi

// > 4 KB -> 4096 byte -> 1024 caratteri da 1 byte ciascuno per le 4 parti dei messaggi

/// dimensione massima del messaggio/file da inviare
#define MSG_MAX_SZ 4096
/// numero parti in cui suddividere il messaggio
#define MSG_PARTS_NUM 4
/// dimensione massima di una porzione di messaggio
#define MSG_BUFFER_SZ (MSG_MAX_SZ / MSG_PARTS_NUM)

/**
 * Rappresenta un messaggio contenente
 * una porzione del contenuto di un file,
 * il numero di file inviati dal client
 * oppure il messaggio "ok" quando il server ha ricevuto il numero di file.
*/
typedef struct msg_struct {

    /// tipo del messaggio: campo usato dalla coda dei messaggi
    long mtype;

    /// PID del mittente del messaggio
    pid_t sender_pid;

    /// Percorso file
    char filePath[BUFFER_SZ+2];

    /// Contenuto messaggio
    char message[MSG_BUFFER_SZ+2];

} msg_struct;


/**
 * Restituisce la prima chiave IPC
 * ottenuta con ftok_IpcKey().
 *
 * @return key_t Chiave IPC
*/
key_t getIpcKey();


/**
 * Restituisce la seconda chiave IPC
 * ottenuta con ftok_IpcKey().
 *
 * @return key_t Chiave IPC
*/
key_t getIpcKey2();


/**
 * Restituisce una chiave IPC generica per il progetto
 * ottenuta con ftok sulla cartella con gli eseguibili.
 *
 * @return key_t Chiave IPC
*/
key_t ftok_IpcKey(char proj_id);

/**
 * @brief Visualizza sullo standard output un messaggio utilizzando la write
 * @param msg messaggio da visualizzare
*/
void print_msg(char * msg);