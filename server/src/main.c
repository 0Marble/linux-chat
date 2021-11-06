
#include "server.h"

static void clearStuff(key_t key)
{
    int mq, sl;
    CALL((mq = msgget(key, 0666 | IPC_CREAT)) < 0, );
    CALL((sl = semget(key, SEM_COUNT, 0666 | IPC_CREAT)) < 0, );

    CALL(semctl(sl, 0, IPC_RMID) < 0, );
    CALL(msgctl(mq, IPC_RMID, NULL) < 0, );
}

extern int allocated, deallocated;

int main(int argc, char **argv)
{
    CALL(argc != 2, printf("Usage: server <port>\n"); return 0);

    Server s = initServer();
    CALL(!startServer(&s, atoi(argv[1])), return 1);
    CALL(!runServer(&s), return 1);
    CALL(!stopServer(&s), return 1);

    printf("Allocated: %d, Deallocated: %d\n", allocated, deallocated);

    return 0;
}