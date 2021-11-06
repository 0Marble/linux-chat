#include "message.h"
#include "users.h"

#include <sys/sem.h>
#include <sys/msg.h>

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#define SEM_COUNT 1
#define SEM_SERVER 0

#define MSG_HANDLER 1
#define MSG_TEXT 'T'
#define MSG_NEW_USER 'U'
#define MSG_USER_LEAVE 'L'

typedef struct Server
{
    List clients;
    uint32_t userCount;

    int acceptorSocket;

    int key;
    int sharedSem;
    int msgQueue;

    bool isRunning;

} Server;

typedef struct IntMessage
{
    long type;
    struct
    {
        char *data;
        uint32_t size;
        int user;
        char type;

        char *msgStart;
    } data;
} IntMessage;

Server initServer();
bool startServer(Server *s, in_port_t port);
bool stopServer(Server *s);

bool runServer(Server *s);

#endif