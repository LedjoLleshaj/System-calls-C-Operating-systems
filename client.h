/// @file client.h
/// @brief Client header file

#pragma once

/**
 * 
 *
 * They are not executed in SIGINTSignalHandler () for the following reason: https://man7.org/linux/man-pages/man7/signal-safety.7.html
 *
 * 
 * @note Does client 0 have to wait for child processes? The specification only indicates that you have to wait for the end message from the server 
 * Currently, it first waits for the finish message and then all children are expected to finish.
 *
*/
void operazioni_client0();

/**
 * SIGINT Handler.
 *
 * It does nothing, it just allows the process to wake up from the pause ().
 *
 * Core functionality is performed by client0 () operations and not here for the following reason: https://man7.org/linux/man-pages/man7/signal-safety.7.html
 *
 * @param sig Integer value corresponding to SIGUSR1
*/
void SIGINTSignalHandler(int sig);

/**
 * @brief SIGUSR1 signal handler: closes client 0 resources and ends its execution.
 *
 * @param sig Integer value corresponding to SIGUSR1
*/
void SIGUSR1SignalHandler(int sig);

/**
 * @brief Divides in 4 part the content of the file.
 *
 * @param fd  File descriptor of the file to divide.
 * @param buf  Buffer to store the content of the file.
 * @param count Number of bytes to read from the file.
 * @param filePath Path of the file to divide.
 * @param parte Number of the part to write.
*/
void dividi(int fd, char *buf, size_t count, char *filePath, int parte);

/**
 * @brief Function performed by each Client i (children of Client 0) to send files to the server.
 *
 * @param filePath  Path of the file to send.
 *
*/
void operazioni_figlio(char * filePath);

/**
 * Stores the path passed as a parameter,
 * manages signals and handler, waits for SIGINT or SIGUSR1 signals.
 *
 * @param argc Number of arguments passed to the program
 * @param argv Array of strings containing the arguments passed to the program
 * @return int Exit code of the whole program
*/
int main(int argc, char * argv[]);


/**
 * Converts integer to string.
 * 
 *
 * @param value  Integer to be converted
 * @return char*  String representation of the integer
*/
char * int_to_string(int value);


/**
 * Return true if two string are equal
 *
 *
 * @param string1 First string to compare
 * @param string2 Second string to compare
 * @return true if the two strings are equal, false otherwise
 *
*/
bool strEquals(char *string1, char *string2);