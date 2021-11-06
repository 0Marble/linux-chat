#include "client.h"

extern int allocated, deallocated;

int main(int argc, char **argv)
{
    CALL(argc != 3, printf("Usage: client <ip> <port>\n"); return 0);

    const char *addr = argv[1];
    in_port_t port = atoi(argv[2]);

    Client c = initClient();
    CALL(!startClient(&c, addr, port), return 1);
    CALL(!runClient(&c), return 1);
    CALL(!stopClient(&c), return 1);
    printf("Allocated: %d, Deallocated: %d\n", allocated, deallocated);
    return 0;
}