#include "users.h"

Client initClient()
{
    Client c;
    c.name = NULL;
    c.socket = 0;
    c.thread = 0;
    c.isOnline = false;
    return c;
}

Link *newLink()
{
    Link *l = NULL;
    CALL((l = (Link *)mallocLog(sizeof(Link))) == NULL, return NULL);
    l->next = NULL;
    l->client = initClient();
    return l;
}

List initList()
{
    List l;
    l.first = l.last = NULL;
    return l;
}

bool pushClient(List *l, Client c)
{
    Link *n = NULL;
    CALL((n = newLink()) == NULL, return false);
    n->client = c;

    if (l->first == NULL)
        l->first = n;
    else
        l->last->next = n;
    l->last = n;
    return true;
}

void clearClientList(List *l)
{
    Link *t = l->first;
    while (t != NULL)
    {
        Link *temp = t->next;
        if (t->client.name)
            freeLog(t->client.name);
        freeLog(t);
        t = temp;
    }
    l->first = l->last = NULL;
}

Link *removeBySocket(List *l, int socket)
{
    Link *prev = NULL;
    for (Link *t = l->first; t != NULL; prev = t, t = t->next)
    {
        if (t->client.socket == socket)
        {
            if (t == l->first)
                l->first = t->next;
            if (t == l->last)
                l->last = prev;

            if (prev != NULL)
                prev->next = t->next;
            return t;
        }
    }

    return NULL;
}