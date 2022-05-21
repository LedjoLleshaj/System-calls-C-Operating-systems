/// @file client.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del CLIENT.

#pragma once

/**
 * Esegue tutte le funzionalita' principali del client 0.
 *
 * Non vengono eseguite in SIGINTSignalHandler() per il seguente motivo: https://man7.org/linux/man-pages/man7/signal-safety.7.html
 *
 * @todo Potrebbe essere necessario ottimizzare l'uso dello HEAP nel client 0.
 *       Attualmente la dimensione massima occupata dal client e':
 *       $$100 file * dim. path + 100 file * dim. pointer$$.
 *       Assumento dim. path massima = 250 caratteri e dim. pointer 4 byte (32 bit):
 *       $$100 * 250 + 100 * 4 = 25400$$
 *       Quindi solo lato client si occupano 25 KByte.
 *       Quando viene creato un client esso eredita momentaneamente la lista creando al massimo un'altra lista concatenata.
 *       Quindi in pratica si occupano:
 *       $$25 * 2 = 50$$
 *       Ovvero 50 KB.
 *
 * @warning Per ottimizzare l'uso dello HEAP nel client 0 si potrebbe
 *          prima cercare e contare quanti file sono presenti senza creare una lista concatenata
 *          e poi ricercare i file e man mano che si trovano file send_me si puo' creare il
 *          processo figlio per inviare il file. Per fare questo BISOGNA sapere se il numero di file
 *          puo' cambiare durante l'esecuzione di questa funzione:
 *          se trovo 3 file e dopo un file viene cancellato cosa succede? <br>
 *          NOTA: questo problema puo' esserci anche nella situazione attuale...
 *
 * @warning Il client 0 deve attendere i processi figlio? La specifica indica solo che bisogna attendere il messaggio di fine dal server...
 *          Attualmente prima si attendere il messaggio di fine e poi si aspetta che tutti i figlio terminino.
 *
 * @warning Il percorso passato al client deve essere assoluto o puo' essere relativo? Se si passa un percorso relativo chdir() fallira' alla seconda esecuzione. <br>
 *          SOLUZIONE: si potrebbe usare un altro chdir() a fine funzione per tornare al percorso di esecuzione iniziale anticipando il chdir() successivo.
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
 * @param a Stringa da confrontare
 * @param b Stringa da confrontare
 * @return true a e b sono uguali
 * @return false a e b sono diverse
 *
*/
bool strEquals(char *a, char *b);