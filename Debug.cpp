#include "Debug.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "Net.h"
#include "Packets.h"

int Debug::debugSock;

void Debug::createDebugPipe(void)
{
    debugSock = createSocket(DEBUG_PORT);
    if (debugSock < 0)
    {
        printf("ERROR: Could not create debug pipe, quitting program.");
        exit(-1);
    }
}

void Debug::writeDebugMessage(const char* format, ...)
{
    struct DebugPacket pkt;
    
    va_list arg;
    va_start(arg, format);
    vsprintf(pkt.str, format, arg);
    va_end(arg);
    
    pkt.strLength = strlen(pkt.str);
    time(&pkt.timestamp);
    
    printf("%s", pkt.str);
    
    if (write(debugSock, &pkt, sizeof(int) + sizeof(time_t) + pkt.strLength + 4) == -1)
    {
        return;
    }
}
