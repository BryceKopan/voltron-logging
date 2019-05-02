#ifndef DEBUG_H
#define DEBUG_H

class Debug
{
public:
    static void createDebugPipe();
    static void writeDebugMessage(const char* format, ...);
    
private:
    static int debugSock;
};

#endif
