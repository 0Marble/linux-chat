#include "client.h"

Client initClient()
{
    Client c;
    c.socket = 0;
    return c;
}

bool startClient(Client *c, const char *ip, in_port_t port)
{
    CALL((c->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0, return false);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = port;
    CALL(inet_aton(ip, &serverAddr.sin_addr) < 0, return false);

    CALL(connect(c->socket,
                 (struct sockaddr *)&serverAddr,
                 sizeof(serverAddr)) < 0,
         return false);

    return true;
}

bool stopClient(Client *c)
{
    CALL(shutdown(c->socket, SHUT_RDWR) < 0, );
    CALL(close(c->socket) < 0, );
    return true;
}

bool isRunning = true;

static void stopHandler(int s)
{
    isRunning = false;
    printf("Received SIGINT\n");
}

static int readStr(char **str, uint32_t *length)
{
    uint32_t allocSize = 0, curSize = 0, nexAlloc = 10;
    char *s = NULL;
    // CALL((s = (char *)mallocLog(allocSize * sizeof(char))) == NULL, return -1);

    bool hadEof = false;

    while (true)
    {
        int c = getchar();

        if (c == '\n')
            break;

        if (curSize == allocSize)
        {
            char *newStr = NULL;
            uint32_t newSize = allocSize + nexAlloc;
            CALL((newStr = (char *)mallocLog(newSize * sizeof(char))) == NULL, return -1);
            allocSize = newSize;
            if (curSize != 0)
            {
                memcpy(newStr, s, allocSize);
                freeLog(s);
            }
            s = newStr;
        }

        s[curSize++] = c;
    }
    s[curSize] = '\0';
    *str = s;
    *length = curSize;

    if (hadEof)
        return 1;

    return 0;
}

static bool sendTextMessage(Client *c, char *msg, uint32_t msgLength)
{
    NetMessage m = initNetMessage();
    m.size = sizeof("message ") + msgLength;
    CALL((m.data = (char *)mallocLog(m.size)) == NULL, return false);
    snprintf(m.data, m.size, "message %s", msg);
    CALL(!sendNetMessage(c->socket, &m), );
    freeLog(m.data);
    return true;
}

static bool writer(Client *c)
{
    bool isRunning = true;

    while (isRunning)
    {
        char *msg = NULL;
        uint32_t length = 0;
        int readRes = 0;

        CALL((readRes = readStr(&msg, &length)) < 0, continue);
        if (readRes == 1)
            isRunning = false;
        else
        {
            CALL(!sendTextMessage(c, msg, length), );
        }
        freeLog(msg);
    }

    CALL(kill(getpid(), SIGINT) < 0, );

    return true;
}

static void *writerF(void *args)
{
    return (void *)writer((Client *)args);
}

static bool proccessMessage(Client *c, NetMessage *m)
{
    char *type = m->data;
    char *msg = NULL;
    for (uint32_t i = 0; i < m->size; i++)
    {
        if (m->data[i] == ' ')
        {
            m->data[i] = '\0';
            msg = &m->data[i + 1];
            break;
        }
    }
    // printf("Type: %s\tMessage: %s\t", type, msg);

    if (strcmp(type, "message") == 0)
    {
        printf("%s\n", msg);
    }
    else if (strcmp(type, "exit") == 0)
    {
        printf("%s quit\n", msg);
    }
    else if (strcmp(type, "accepted") == 0)
    {
        printf("Accepted\n");
    }
    else if (strcmp(type, "new") == 0)
    {
        printf("%s joined\n", msg);
    }
    else
    {
        printf("Unknown message!\n");
    }

    freeLog(m->data);
    return true;
}

static bool handler(Client *c)
{
    while (true)
    {
        NetMessage m = initNetMessage();
        bool hadMessage = false;
        CALL(!receiveNetMessage(c->socket, &m, &hadMessage), continue);

        if (!hadMessage)
            continue;

        CALL(!proccessMessage(c, &m), continue);
    }

    return true;
}

static void *handlerF(void *args)
{
    return (void *)handler((Client *)args);
}

static bool setName(Client *c)
{
    printf("Enter name: ");
    char *name = NULL;
    uint32_t nameSize = 0;

    int readRes = 0;
    CALL((readRes = readStr(&name, &nameSize)) < 0, return false);
    if (readRes == 1)
        return false;

    NetMessage nameMsg = initNetMessage();
    nameMsg.size = nameSize + sizeof("name ");
    CALL((nameMsg.data = mallocLog(nameMsg.size)) == NULL, return false);
    snprintf(nameMsg.data, nameMsg.size, "name %s", name);

    CALL(!sendNetMessage(c->socket, &nameMsg), return false);
    freeLog(nameMsg.data);

    freeLog(name);

    return true;
}

bool runClient(Client *c)
{
    CALL(signal(SIGINT, stopHandler) == SIG_ERR, return false);

    CALL(!setName(c), return false);

    pthread_t handlerThread, writerThread;
    P_CALL(pthread_create(&handlerThread, NULL, handlerF, (void *)c), return false);
    P_CALL(pthread_create(&writerThread, NULL, writerF, (void *)c), return false);

    while (isRunning)
    {
    }

    printf("Exiting..\n");
    P_CALL(pthread_cancel(writerThread), );
    P_CALL(pthread_cancel(handlerThread), );

    NetMessage m = initNetMessage();
    m.data = "quit ";
    m.size = sizeof("quit ");
    CALL(!sendNetMessage(c->socket, &m), );

    return true;
}