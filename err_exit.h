/// @file err_exit.h
/// @brief Contains functions to exit the program with an error message.

#pragma once

/// @brief Prints the error message of the last failed
///         system call and terminates the calling process.
void errExit(const char *msg);