/// @file client.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del CLIENT.

#pragma once

/**
 * Esegue tutte le funzionalita' principali del client 0.
 *
 * Non vengono eseguite in SIGINTSignalHandler() per il seguente motivo: https://man7.org/linux/man-pages/man7/signal-safety.7.html
 *
 * 
 * @note Il client 0 deve attendere i processi figlio? La specifica indica solo che bisogna attendere il messaggio di fine dal server...
 *          Attualmente prima si attendere il messaggio di fine e poi si aspetta che tutti i figlio terminino.
 *
*/
void operazioni_client0();

/**
 * Handler del segnale SIGINT.
 *
 * Non fa niente, permette solo al processo di risvegliarsi dal pause().
 *
 * Le funzionalita' principali vengono eseguite da operazioni_client0() e non qui per il seguente motivo: https://man7.org/linux/man-pages/man7/signal-safety.7.html
 *
 * @param sig Valore intero corrispondente a SIGINT
*/
void SIGINTSignalHandler(int sig);

/**
 * @brief Handler del segnale SIGUSR1: chiude le risorse del client 0 e termina la sua esecuzione.
 *
 * @param sig Valore intero corrispondente a SIGUSR1
*/
void SIGUSR1SignalHandler(int sig);

/**
 * @brief Divide in 4 parti il contenuto del file da inviare al server.
 *
 * @param fd File descriptor del file da inviare
 * @param buf Buffer in cui memorizzare la porzione del messaggio
 * @param count Dimensione della porzione di messaggio
 * @param filePath Percorso del file da suddividere
 * @param parte Numero identificativo della porzione di messaggio
*/
void dividi(int fd, char *buf, size_t count, char *filePath, int parte);

/**
 * @brief Funzione eseguita da ogni Client i (figli di Client 0) per mandare i file al server.
 *
 * @param filePath Percorso del file che il client deve suddividere e mandare al server.
 *
*/
void operazioni_figlio(char * filePath);

/**
 * Memorizza il percorso passato come parametro,
 * gestisce segnali e handler, attende i segnali SIGINT o SIGUSR1.
 *
 * @param argc Numero argomenti passati da linea di comando (compreso il nome dell'eseguibile)
 * @param argv Array di argomenti passati da linea di comando
 * @return int Exit code dell'intero programma
*/
int main(int argc, char * argv[]);


/**
 * Converte un intero in stringa.
 * > E' necessaria la free() dopo aver terminato l'uso della stringa.
 *
 * @param value Valore intero da convertire in stringa
 * @return char* Stringa contenente il valore <value>
 *
*/
char * int_to_string(int value);


/**
 * Restituisce vero se due stringhe sono uguali
 *
 * @param string1 Stringa da confrontare
 * @param string2 Stringa da confrontare
 * @return true string1 e string2 sono uguali
 * @return false string1 e string2 sono diverse
 *
*/
bool strEquals(char *string1, char *string2);