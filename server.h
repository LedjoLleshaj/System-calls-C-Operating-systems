/// @file server.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del SERVER.

#pragma once

#include "defines.h"

/**
 * @brief Chiude tutte le IPC e termina
 *
 * @param sig Intero che rappresenta il segnale catturato dalla funzione
 */
void SIGINTSignalHandler(int sig);

/**
 * Converte stringa in intero.
 *
 * @param string Stringa da convertire in intero
 * @return int Valore intero ottenuto convertendo la stringa in input
*/
int string_to_int(char * string);

/**
 * Aggiunge un messaggio alla matrice buffer.
 * Il buffer verra' usato per recuperare i pezzi di file
 * quando verra' ricostruito il file di output.
 *
 * @param a Messaggio da inserire nel buffer
 * @param righe Numero di righe nella matrice
*/
void aggiungiAMatrice(message_t a,int righe);

/**
 * Trova nella matrice buffer 4 pezzi di un file
 * e poi li usa per creare e scrivere un file di output.
 *
 * @param righe Numero di righe nella matrice
*/
void findAndMakeFullFiles(int righe);

/**
 * Costruisce la stringa da scrivere nel file di output.
 *
 * @param a Messaggio contenente il pezzo di file arrivato dal client
 * @return char* Stringa pronta per essere scritta su file
*/
char * costruisciStringa(message_t a);

/**
 * Esegue operazioni principali del server.
 *
 * terminazione effettuata con SIGINT: Al termine chiudi tutte le IPC.
*/
int main(int argc, char * argv[]);