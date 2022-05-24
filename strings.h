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
 * @brief Restituisce vero se la stringa filename inizia con la sotto stringa "_sendme" e !contains "_out"
 *
 * @param filename
 * @param sendme
 * @param out
 * @return true
 * @return false
 */
bool StartsWith_EndsWith(const char *filename, const char *sendme,const char *out);