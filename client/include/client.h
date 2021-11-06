#include "call.h"
#include "message.h"

#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

typedef struct Client
{
    int socket;
} Client;

Client initClient();
bool startClient(Client *c, const char *ip, in_port_t port);
bool runClient(Client *c);
bool stopClient(Client *c);

#endif