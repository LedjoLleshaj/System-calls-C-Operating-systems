/// @file shared_memory.h
/// @brief Shared memory functions
#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

/**
 * @brief Obtains and ID and creates a shared memory segment.
 *
 * @param shmKey IPC key fo shared memory segment.
 * @param size Size of shared memory segment.
 * @return ID of shared memory segment.
 */
int sharedMemoryGet(key_t shmKey, size_t size);

/**
 * @brief Retrieve the shared memory pointer by attaching it.
 *
 * @param shmid ID of shared memory segment.
 * @param shmflg Flag
 * @return void* pointer to shared memory segment.
 */
void *sharedMemoryAttach(int shmid, int shmflg);

/**
 * @brief Detach the shared memory from the address space.
 *
 * @param ptr_sh pointer to shared memory segment.
 */
void sharedMemoryDetach(void *ptr_sh);

/**
 * @brief Removes the shared memory segment.
 *
 * @param shmid ID of shared memory segment.
 */
void sharedMemoryRemove(int shmid);