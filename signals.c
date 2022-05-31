/// @file signals.c
/// @brief Signal handling functions
#include <signal.h>

#include "signals.h"


void allowOnlySIGINT_SIGUSR1(){
    sigset_t signalSet, prevSignalSet;
    sigfillset(&signalSet);                                // initialize signalSet which contains all signals
    sigdelset(&signalSet, SIGINT);                         // delete SIGINT from the list of blocked signals
    sigdelset(&signalSet, SIGUSR1);                        // delete SIGUSR1 from the list of blocked signals
    sigprocmask(SIG_SETMASK, &signalSet, &prevSignalSet);  // imposed new signal mask
}


void blockAllSignals(){
    sigset_t signalSet, prevSignalSet;
    sigfillset(&signalSet);                                // initialize signalSet which contains all signals
    sigprocmask(SIG_SETMASK, &signalSet, &prevSignalSet);  // imposed new signal mask
}