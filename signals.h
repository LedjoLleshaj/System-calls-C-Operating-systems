/// @file signals.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEGNALI.

#pragma once

/**
 * Blocca tutti i segnali meno SIGINT e SIGUSR1:
 * - creando un set di tutti i segnali
 * - togliendo SIGINT e SIGUSR1 dal set
 * - impostando la maschera per bloccare i segnali del set
 *
*/
void onlySIGINT_SIGUSR1();


/**
 * Blocca tutti i segnali:
 * - creando un set di tutti i segnali
 * - impostando la maschera per bloccare i segnali del set
*/
void blockAllSignals();