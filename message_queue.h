
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione delle CODE DEI MESSAGGI.

#pragma once

#include <sys/msg.h>

/**
 * @brief Restituisce statistiche della coda dei messaggi
 *
 * @param msqid Identifier coda dei messaggi
 * @return struct msqid_ds Struttura con statistiche della coda dei messaggi
 */
struct msqid_ds msqGetStats(int msqid);

/**
 * @brief Imposta nuove configurazioni sulla coda dei messaggi
 *
 * @param msqid Identifier coda dei messaggi
 * @param ds Struttura con nuove statistiche della coda dei messaggi
 */
void msqSetStats(int msqid, struct msqid_ds ds);