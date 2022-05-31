/// @file defines.h
/// @brief Contains the definitions used in the program.

#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>

/// Buffer used from getcwd()
#define BUFFER_SZ 255

extern char CURRENT_DIRECTORY[BUFFER_SZ];

/// FIFO path
#define FIFO1_PATH "/tmp/fifo1_file.txt"
/// FIFO path
#define FIFO2_PATH "/tmp/fifo2_file.txt"
/// mtype message that contains the total number of files "sendme_"
#define FILE_NR_MTYPE 1
/// mtype message that contains first part of the file "sendme_"
#define FIFO1_PART 2
/// mtype message that contains second part of the file "sendme_"
#define FIFO2_PART 3
/// mtype message that contains third part of the file "sendme_"
#define MSGQUEUE_PART 4
/// mtype message that contains fourth part of the file "sendme_"
#define SHARED_MEMORY_PART 5
/// mtype message that contains finish msg from server
#define DONE 6
/// mtype message that contains the value used to initialize
#define EMPTY_MTYPE -1

/// maximum number of messages per communication channel
#define MAX_MSG_PER_CHANNEL 50

// > 4 KB -> 4096 byte -> 1024 characters of 1 byte each for the 4 parts of the messages

/// maximum size of the message / file to send
#define MSG_MAX_SZ 4096
/// number of parts into which to divide the message
#define MSG_PARTS_NUM 4
/// maximum size of a message part
#define MSG_BUFFER_SZ (MSG_MAX_SZ / MSG_PARTS_NUM)

/**
 * Rapresents a message containing
 * a portion of the content of a file,
 * the number of files sent by the client
 * or the "ok" message when the server has received the file number.
 */
typedef struct message_t
{

    /// message type: field used by the message queue
    long mtype;

    /// PID of the message sender
    pid_t sender_pid;

    /// file path
    char file_path[BUFFER_SZ + 2];

    /// Message content
    char msg_body[MSG_BUFFER_SZ + 2];

} message_t;

/**
 * Returns the first IPC key
 * obtained with get_project_ipc_key ().
 *
 * @return key_t IPC key
 * @see get_project_ipc_key ()
 *
 */
key_t get_ipc_key();

/**
 * Returns the second IPC key
 * obtained with get_project_ipc_key ().
 *
 * @return key_t IPC key
 *
 */
key_t get_ipc_key2();

/**
 * Returns a generic IPC key for the project
 * obtained with ftok on the folder with the executables.
 *
 * @return key_t IPC key
 *
 */
key_t get_project_ipc_key(char proj_id);

/**
 * @brief Return true if the arrya is all true
 *
 * @param arr array of booleans
 * @param len length of the array
 * @return true if all the elements of the array are true
 * @return false otherwise
 */
bool arrayContainsAllTrue(bool arr[], int len);

/**
 * @brief Makes a file descriptor blocking or non-blocking.
 *
 * @param fd file descriptor
 * @param blocking 0: non blocking, 1: blocking
 * @return int 0 if it fails
 */
int blockFD(int fd, int blocking);

/**
 * @brief Displays a message on standard output using write
 * @param msg message to display
 */
void print_msg(char *msg);