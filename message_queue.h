/// @brief Message queue functions

#pragma once

#include <sys/msg.h>

/**
 * @brief Returns message queue statistics
 *
 * @param msqid - message queue id
 * @return struct msqid_ds - Structure with message queue statistics
 */
struct msqid_ds msqGetStats(int msqid);

/**
 * @brief Set new configurations on the message queue
 *
 * @param msqid - message queue id
 * @param ds - new configurations
 */
void msqSetStats(int msqid, struct msqid_ds ds);