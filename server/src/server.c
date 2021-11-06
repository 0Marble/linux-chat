#include "server.h"

static bool sUp(int semList, uint16_t semId)
{
    struct sembuf op;
    op.sem_num = semId;
    op.sem_op = 1;
    CALL(semop(semList, &op, 1) < 0, return false);
    return true;
}

static bool sDown(int semList, uint16_t semId)
{
    struct sembuf op;
    op.sem_num = semId;
    op.sem_op = -1;
    CALL(semop(semList, &op, 1) < 0, return false);
    return true;
}

Server initServer()
{
    Server s;
    s.clients = initList();
    s.acceptorSocket = 0;
    s.sharedSem = 0;
    s.key = 0;
    s.isRunning = false;
    s.userCount = 0;
    s.msgQueue = 0;

    return s;
}

pthread_mutex_t userListLock;

bool startServer(Server *s, in_port_t port)
{
    s->key = port;
    const char *addr = "127.0.0.1";
    CALL((s->acceptorSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0, return false);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = port;
    CALL(inet_aton(addr, &serverAddr.sin_addr) < 0, return false);
    CALL(bind(s->acceptorSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0, return false);
    P_CALL(pthread_mutex_init(&userListLock, NULL), return false);

    CALL(listen(s->acceptorSocket, 10) < 0, return false);
    s->isRunning = true;
    return true;
}

bool stopServer(Server *s)
{
    P_CALL(pthread_mutex_destroy(&userListLock), );
    clearClientList(&s->clients);
    *s = initServer();
    return true;
}

static bool sendToEveryone(Server *s, NetMessage *m, int ignoredSocket)
{
    for (Link *l = s->clients.first; l != NULL; l = l->next)
    {
        if (l->client.socket == ignoredSocket)
            continue;

        CALL(!sendNetMessage(l->client.socket, m), continue);
    }

    return true;
}

static bool sendText(Server *s, char *text, Link *sender)
{
    NetMessage m = initNetMessage();
    m.size = sizeof("message ") + strlen(sender->client.name) + sizeof(" : ") + strlen(text);
    CALL((m.data = (char *)mallocLog(m.size)) == NULL, return false);
    snprintf(m.data, m.size, "message %s : %s", sender->client.name, text);
    CALL(!sendToEveryone(s, &m, sender->client.socket), );
    freeLog(m.data);

    return true;
}

static bool sendQuitMessage(Server *s, Link *client)
{
    removeBySocket(&s->clients, client->client.socket);
    CALL(!shutdown(client->client.socket, SHUT_RDWR) < 0, );
    CALL(!close(client->client.socket) < 0, );

    NetMessage userExitMsg = initNetMessage();

    if (client->client.name)
    {
        userExitMsg.size = sizeof("exit ") + strlen(client->client.name);
        CALL((userExitMsg.data = (char *)mallocLog(userExitMsg.size)) == NULL, );
        snprintf(userExitMsg.data, userExitMsg.size, "exit %s", client->client.name);
        CALL(!sendToEveryone(s, &userExitMsg, client->client.socket), );
        freeLog(userExitMsg.data);
    }
    else
    {
        userExitMsg.size = sizeof("exit ");
        userExitMsg.data = "exit ";
        CALL(!sendToEveryone(s, &userExitMsg, client->client.socket), );
    }

    if (client->client.name)
        freeLog(client->client.name);
    freeLog(client);

    return true;
}

static bool sendRenameMessage(Server *s, Link *client, char *name)
{
    NetMessage userNameMsg = initNetMessage();
    uint32_t nameSize = strlen(name) + sizeof("new ");
    userNameMsg.size = nameSize;

    CALL((userNameMsg.data = (char *)mallocLog(nameSize)) == NULL, return false);
    snprintf(userNameMsg.data, nameSize, "new %s", name);
    CALL(!sendToEveryone(s, &userNameMsg, client->client.socket), );

    snprintf(userNameMsg.data, nameSize, "%s", name);
    client->client.name = userNameMsg.data;

    return true;
}

static bool proccessMessage(Server *s, NetMessage *m, Link *client, bool *hasQuit)
{
    char *type = m->data;
    char *msg = NULL;
    bool hadSpace = false, hadArg = false;
    uint32_t spaceIndex = 0;
    for (uint32_t i = 0; i < m->size; i++)
    {
        if (isspace(m->data[i]))
        {
            if (!hadSpace)
            {
                hadSpace = true;
                spaceIndex = i;
                m->data[i] = '\0';
                msg = &m->data[i + 1];
            }
        }
        else if (hadSpace && !hadArg)
        {
            hadArg = true;
        }
    }

    printf("Type: %s\tMessage: %s\t", type, msg);

    P_CALL(pthread_mutex_lock(&userListLock), return false);
    if (strcmp(type, "message") == 0)
    {
        if (hadSpace)
            m->data[spaceIndex] = ' ';
        CALL(!sendText(s, msg, client), );
    }
    else if (strcmp(type, "quit") == 0)
    {
        CALL(!sendQuitMessage(s, client), );

        *hasQuit = true;
    }
    else if (strcmp(type, "name") == 0)
    {
        if (hadArg)
        {
            CALL(!sendRenameMessage(s, client, msg), );
        }
    }
    else
    {
        printf("Unknown message!\n");
    }
    P_CALL(pthread_mutex_unlock(&userListLock), );

    freeLog(m->data);
    return true;
}

typedef struct HandlerArgs
{
    Server *s;
    Link *c;
} HandlerArgs;

static bool handler(HandlerArgs *a)
{
    P_CALL(pthread_mutex_lock(&userListLock), );
    P_CALL(pthread_mutex_unlock(&userListLock), );

    Server *s = a->s;
    Link *client = a->c;
    freeLog(a);

    bool hasQuit = false;
    while (!hasQuit)
    {
        bool hadMessage = false;
        NetMessage m = initNetMessage();

        CALL(!receiveNetMessage(client->client.socket, &m, &hadMessage), continue);
        // printf("hadMessage: %d\n", hadMessage);
        if (!hadMessage)
            continue;

        CALL(!proccessMessage(s, &m, client, &hasQuit), continue);
    }

    return true;
}

static void *handlerF(void *args)
{
    return (void *)handler(args);
}

static bool acceptor(Server *s)
{
    while (true)
    {
        struct sockaddr clientAddr;
        socklen_t clientAddrSize = 0;

        int clientSocket = 0;
        CALL((clientSocket = accept(s->acceptorSocket, &clientAddr, &clientAddrSize)) < 0, continue);

        P_CALL(pthread_mutex_lock(&userListLock), continue);

        Client c = initClient();
        c.socket = clientSocket;
        printf("New client on socket %d\n", clientSocket);

        CALL(!pushClient(&s->clients, c), );
        HandlerArgs *args = NULL;
        CALL((args = (HandlerArgs *)mallocLog(sizeof(HandlerArgs))) == NULL, continue);
        args->s = s;
        args->c = s->clients.last;
        P_CALL(pthread_create(&c.thread, NULL, handlerF, (void *)args), continue);

        NetMessage m = initNetMessage();
        m.data = "accepted ";
        m.size = sizeof("accepted ");
        CALL(!sendNetMessage(clientSocket, &m), );

        P_CALL(pthread_mutex_unlock(&userListLock), continue);
    }

    return true;
}

static void *acceptorF(void *args)
{
    return (void *)acceptor((Server *)args);
}

bool isRunning = true;

static void closeHandler(int s)
{
    isRunning = false;
    printf("Received SIGINT\n");
}

bool runServer(Server *s)
{
    CALL(signal(SIGINT, closeHandler) == SIG_ERR, return false);
    pthread_t acceptorThread;

    P_CALL(pthread_create(&acceptorThread, NULL, acceptorF, (void *)s), return false);
    while (isRunning)
    {
    }

    P_CALL(pthread_cancel(acceptorThread), return false);

    return true;
}
