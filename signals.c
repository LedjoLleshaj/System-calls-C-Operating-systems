/// @file signals.c
/// @brief Contiene le funzioni specifiche per la gestione dei SEGNALI.
#include <signal.h>

#include "signals.h"


void onlySIGINT_SIGUSR1(){

    sigset_t signalSet, prevSignalSet;
    // inizializzo signalSet che contiene tutti i segnali
    sigfillset(&signalSet);     
    // elimino SIGINT dalla lista dei segnali bloccati                           
    sigdelset(&signalSet, SIGINT); 
    // elimino SIGUSR1 dalla lista dei segnali bloccati                        
    sigdelset(&signalSet, SIGUSR1);     
    // imposto nuova maschera dei segnali           
    sigprocmask(SIG_SETMASK, &signalSet, &prevSignalSet);  
}

void blockAllSignals(){
    sigset_t signalSet, prevSignalSet;
    // inizializzo signalSet che contiene tutti i segnali
    sigfillset(&signalSet);
    // imposto nuova maschera dei segnali                                
    sigprocmask(SIG_SETMASK, &signalSet, &prevSignalSet);  
}