#include "call.h"

#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#define BUFF_SIZE 256

typedef struct NetMessage
{
    char *data;
    uint32_t size;
} NetMessage;

bool expandArray(void **array, uint32_t *curSize, uint32_t expandBy);
bool receiveNetMessage(int fromSocket, NetMessage *m, bool *hadMessage);
bool sendNetMessage(int toSocket, NetMessage *m);
NetMessage initNetMessage();

#endif