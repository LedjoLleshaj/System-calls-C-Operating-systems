/// @file server.h
/// @brief Server functions and neccecary data structures from defines.h
#pragma once

#include "defines.h"

/**
 * @brief CLose all the IPCs and exit the program
 *
 * @param sig - signal number
 */
void SIGINTSignalHandler(int sig);

/**
 * @brief Convert a string to int
 *
 * @param string - string to convert
 * @return int - converted string
 */
int string_to_int(char *string);

/**
 * @brief Adds a message to the buffer array.
 * The buffer will be used to retrieve the pieces of files
 * when the output file is rebuilt.
 *
 * @param a message to insert in array
 * @param righe - number of rows in the array
 */
void addToMatrix(message_t a, int righe);

/**
 * @brief Find 4 pieces of a file in the buffer matrix
 * and then uses them to create and write an output file.
 *
 * @param righe - number of rows in the array
 *
 */
void findAndMakeFullFiles(int righe);

/**
 * @brief Constructs the string to be written to the output file.
 *
 * @param a Message that came from server and that has to be written
 * @return char* - string to be written to the output file
 *
 */
char *prepareString(message_t a);

/**
 * @brief Execute operations
 *  termination performed with SIGINT: At the end close all IPCs.
 */
int main(int argc, char *argv[]);