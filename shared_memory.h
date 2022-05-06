/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once

#include <sys/types.h>

/**
 * @brief Ottiene l'id o crea una memoria condivisa.
 *
 * @param shmKey Chiave IPC della memoria condivisa
 * @param size Dimensione della memoria condivisa
 * @return int Identificativo della memoria condivisa
*/
int alloc_shared_memory(key_t shmKey, size_t size);


/**
 * @brief Recupera il puntatore alla memoria condivisa aggiungendola allo spazio degli indirizzi.
 *
 * @param shmid Identificativo della memoria condivisa
 * @param shmflg Flag
 * @return void* Puntatore per accedere alla memoria condivisa
*/
void * get_shared_memory(int shmid, int shmflg);


/**
 * @brief Scollega la memoria condivisa dallo spazio degli indirizzi.
 *
 * @param ptr_sh Puntatore per accedere alla memoria condivisa
*/
void free_shared_memory(void *ptr_sh);


/**
 * @brief Elimina la memoria condivisa
 *
 * @param shmid Identificativo della memoria condivisa
*/
void remove_shared_memory(int shmid);