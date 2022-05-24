/// @file files.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei FILE.

#pragma once

/**
 * Nodo di una lista concatenata di filePath.
*/
typedef struct files_list{
    /// Percorso file
    char *path;
    /// Puntatore al prossimo nodo
    struct files_list *next;
} files_list;

/**
 * @brief Visualizza il contenuto della lista concatenata di filePath.
 *
 * @param head Nodo di testa della coda
 */
void print_list(files_list * head);

/**
 * @brief Aggiunge in fondo alla coda un nuovo filePath
 *
 * @param head Nodo di testa della coda
 * @param path filePath da aggiungere in fondo
 * @return files_list* Nodo di testa della coda (se head == NULL verra' creato il primo nodo)
*/
files_list * append(files_list * head, char * path);

/**
 * @brief Libera la memoria dello HEAP occupata dalla lista di filePath
 *
 * @param head Nodo di testa della coda
 */
void free_list(files_list * head);

/**
 * @brief Conta il numero di file nella lista concatenata di filePath
 *
 * @param head Nodo di testa della coda
 * @return int Numero di file
 */
int count_files(files_list * head);

/**
 * @brief Restituisce la dimensione in byte del file con percorso filePath.
 *
 * @param filePath Percorso del file
 * @return long Dimensione del file in byte
 */
long getFileSize(char * filePath);

/**
 * @brief Restituisce 1 se il percorso ha la dimensione <= 4 KB, 0 altrimenti.
 *
 * @param filePath Percorso del file
 * @return int Vale 1 se il percorso ha la dimensione <= 4 KB, 0 altrimenti
*/
int checkFileSize(char * filePath);

/**
 * @brief Restituisce 1 se il nome del file inizia con "sendme_" e !contains "_out", altrimenti 0.
 *
 * @param fileName Percorso del file
 * @return int Vale 1 se il nome del file inizia con "sendme_" e !contains "_out", altrimenti 0
*/
int checkFileName(char * fileName);

/**
 * Ricerca ricorsivamente nel percorso searchPath i file che inizianano con "sendme_" e !contains "_out"
 * e che hanno la dimensione <= 4 KB.
 *
 * @param searchPath Percorso in cui ricercare i file
 * @param head Testa della lista concatenata in cui verranno restituiti i percorsi dei file che hanno i requisiti richiesti
 * @return files_list* Testa della lista concatenata con i percorsi dei file che hanno i requisiti richiesti
*/
files_list * find_sendme_files(char *searchPath, files_list * head);