/// @file signals.h
/// @brief Signal handling functions
///

#pragma once

/**
 * Block all signals except SIGINT and SIGUSR1:
 * - creating a set of all signals
 * - removing SIGINT and SIGUSR1 from the set
 * - setting the mask to block the signals of the set
 *
 */
void allowOnlySIGINT_SIGUSR1();

/**
 * Block all signals:
 * - creating a set of all signals
 * - setting the mask to block the signals of the set
 */
void blockAllSignals();