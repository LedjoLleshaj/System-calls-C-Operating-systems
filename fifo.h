/// @file fifo.h
/// @brief Contains FIFO structs and functions.

#pragma once


/**
 * @brief Create or get a FIFO IPC using mkfifo.
 *
 * @param path Path to the FIFO
*/
void make_fifo(char * path);


/**
 * @brief Makes it easier to create a FIFO (or obtains an existing one) in read or write mode.
 *
 * @param path Path to the FIFO
 * @param mode 'r' (read) or 'w' (write)
 * @return int File descriptor
 */
int create_fifo(char * path, char mode);