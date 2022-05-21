/// @file strings.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione delle STRINGHE.

#pragma once

#include <stdbool.h>

/**
 * @brief Concatena la cartella directory al percorso path.
 *
 * @param path Percorso root a cui aggiungere directory
 * @param directory Nome cartella da aggiungere al path
 * @return size_t Dimensione caratteri di directory
 */
size_t append2Path(char *path, char *directory);

/**
 * @brief Restituisce vero se la stringa a inizia con la sotto stringa b
 *
 * @param a
 * @param b
 * @return true
 * @return false
 */
bool StartsWith(const char *a, const char *b);