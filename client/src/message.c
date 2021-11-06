#include "message.h"

NetMessage initNetMessage()
{
    NetMessage m;
    m.data = NULL;
    m.size = 0;
    return m;
}

bool expandArray(void **array, uint32_t *curSize, uint32_t expandBy)
{
    uint32_t newSize = *curSize + expandBy;
    void *newArray = NULL;
    CALL((newArray = mallocLog(newSize)) == NULL, return false);

    if (*curSize != 0)
    {
        memcpy(newArray, *array, *curSize);
        freeLog(*array);
    }
    *array = newArray;
    *curSize = newSize;

    return true;
}

bool receiveNetMessage(int fromSocket, NetMessage *m, bool *hadMessage)
{
    char buff[BUFF_SIZE] = {0};

    while (true)
    {
        memset(buff, 0, BUFF_SIZE);
        errno = 0;
        int readBytes = recv(fromSocket, buff, BUFF_SIZE, 0);
        if (readBytes < 0)
        {
            CALL(errno != EAGAIN && errno != EWOULDBLOCK, return false);
            readBytes = 0;
        }

        if (readBytes == 0)
        {
            *hadMessage = false;
            break;
        }

        CALL(!expandArray((void **)&m->data, &m->size, readBytes), return false);
        memcpy(m->data - m->size + readBytes, buff, readBytes);

        if (buff[readBytes - 1] == '\0')
        {
            *hadMessage = true;
            break;
        }
        //man, lesson pdf and the whole internet have blatantly lied,
        //recv() is not in fact blocking or why would it keep saying
        //readBytes==0??
    }

    return true;
}

bool sendNetMessage(int toSocket, NetMessage *m)
{
    uint32_t i = 0;
    while ((i + 1) * BUFF_SIZE < m->size)
    {
        char *sendingFrom = m->data + i * BUFF_SIZE;
        i++;
        CALL(send(toSocket, sendingFrom, BUFF_SIZE, 0) < 0, return false);
    }

    char buff[BUFF_SIZE + 1] = {0};
    memcpy(buff, m->data + i * BUFF_SIZE, m->size - i * BUFF_SIZE);
    CALL(send(toSocket, buff, BUFF_SIZE, 0) < 0, return false);

    return true;
}