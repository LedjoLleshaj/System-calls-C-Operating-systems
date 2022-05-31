/// @file semaphore.h
/// @brief Semaphore functions
#pragma once

/**
 * Union used from the system call semctl().
 */
union semun
{
    /// Used if working on a single semaphore.
    /// Used by the SETVAL operation
    int val;

    /// Used to work on the global state of the semaphore.
    /// Used by the IPC_STAT and IPC_SET operations
    struct semid_ds *buf;

    /// to perform operations on all semaphores.
    /// Used by the GETALL and SETALL operations
    unsigned short *array;
};

/**
 * @brief Obtains the id of a created semaphore.
 *
 * @param key IPC key
 * @param n_sem number of semaphores
 */
int semGetID(key_t key, int n_sem);

/**
 * @brief Create a set of semaphores.
 *
 * @param key IPC key
 * @param n_sem Numero di semafori
 */
int semGetCreate(key_t key, int n_sem);

/**
 * @brief Wrapper function for manipulating the values of a set of semaphores.
 *
 * @param semid  ID of the set of semaphores
 * @param sem_num  Index of the semaphore to be manipulated
 * @param sem_op Operation to be performed on the semaphore
 */
void semOp(int semid, unsigned short sem_num, short sem_op);

/**
 * Wrapper function for manipulating the values of a set of semaphores in a non-blocking way.
 * Returns -1 if the semaphore attempted to block the process, 0 otherwise.
 *
 * @param semid ID of the set of semaphores
 * @param sem_num Index of the semaphore to be manipulated
 * @param sem_op Operation to be performed on the semaphore
 *
 * @return -1 if the semaphore attempted to block the process, 0 otherwise.
 */
int semOp_NOWAIT(int semid, unsigned short sem_num, short sem_op);

/**
 * Waits for the sem_num semaphore to reach zero.
 * Before using it to synchronize processes call semWait on the same semaphore.
 *
 * @param semid ID of the set of semaphores
 * @param sem_num Index of the semaphore to be manipulated
 *
 */
void semWaitZero(int semid, int sem_num);

/**
 * @brief It executes the wait on semaphore sem_num: it decreases the value by 1 and eventually puts the process on hold.
 *
 * @param semid ID of the set of semaphores
 * @param sem_num Index of the semaphore to be manipulated
 */
void semWait(int semid, int sem_num);

/**
 * Performs the non-blocking wait on sem_num semaphore: attempts to decrease its value by 1.
 * Returns -1 if the semaphore attempted to block the process, 0 otherwise.
 *
 * @param semid ID of the set of semaphores
 * @param sem_num Index of the semaphore to be manipulated
 * @return -1 if the semaphore attempted to block the process, 0 otherwise.
 */
int semWait_NOWAIT(int semid, int sem_num);

/**
 * @brief Executes the signal on the semaphore sem_num: increases the value by 1.
 *
 * @param semid ID of the set of semaphores
 * @param sem_num Index of the semaphore to be manipulated
 */
void semSignal(int semid, int sem_num);

/**
 * @brief Initializes the semaphore value sem_num to the value val.
 *
 * @param semid ID of the set of semaphores
 * @param sem_num Index of the semaphore to be manipulated
 * @param val Value to be set
 */
void semSetVal(int semid, int sem_num, int val);

/**
 * @brief Initializes the values of the semid semaphore set to the values in values.
 *
 * @param semid ID of the set of semaphores
 * @param values Array in which semaphore values are stored
 */
void semSetAll(int semid, short unsigned int values[]);

/**
 * @brief Clears the set of semaphores by waking any waiting processes.
 *
 * @param semid ID of the set of semaphores
 */
void semDelete(int semid);

/**
 * @brief Set permissions on the semaphore set.
 *
 * @param semid ID of the set of semaphores
 * @param arg Permissions to be set
 */
void semSetPerm(int semid, struct semid_ds arg);