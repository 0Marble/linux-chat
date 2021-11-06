#include "call.h"
#include "message.h"

#ifndef CHAT_USERS_H
#define CHAT_USERS_H

typedef struct Client
{
    int socket;
    char *name;
    pthread_t thread;
    bool isOnline;
} Client;

typedef struct Link
{
    struct Link *next;
    Client client;
} Link;

typedef struct List
{
    Link *first, *last;
} List;

Client initClient();
List initList();
bool pushClient(List *l, Client c);
void clearClientList(List *l);
Link *removeBySocket(List *l, int socket);

#endif