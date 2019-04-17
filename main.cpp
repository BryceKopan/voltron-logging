#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SQL_HOST "localhost"
#define SQL_USER "root"
#define SQL_PASSWORD ""
#define SQL_DB "Voltron"

#include "SQLWrapper.h"

using namespace std;

struct BatteryPacket
{
    int cellNum;
    
    float charge;
};

#define BATTERY_PORT 12001

int main(int argc, const char **argv)
{
    SQLWrapper::ConnectToDatabase(SQL_HOST, SQL_USER, SQL_PASSWORD, SQL_DB);
    sql::PreparedStatement* preparedQuery = SQLWrapper::GetPreparedStatement("INSERT INTO batterydata(cell, charge) VALUES (?, ?)");
    
    
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        perror("[Battery] Socket creation failure");
        return 0;
    }
    
    int one = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(BATTERY_PORT);
    
    if (bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("[Battery] Socket bind failure");
        return 0;
    }
    
    struct ip_mreq group;
    group.imr_multiaddr.s_addr = inet_addr("224.0.0.155");
    group.imr_interface.s_addr = inet_addr("127.0.0.1");
    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
    {
        perror("[Battery] Socket group creation failure");
        return 0;
    }
    
    printf("[Battery] Socket open\n");
    
    while(1)
    {
        struct BatteryPacket pkt;
        
        read(sockfd, &pkt, sizeof(pkt));
        
        printf("%i - %f\n", pkt.cellNum, pkt.charge);
        
        preparedQuery->setString(1, std::to_string(pkt.cellNum));
        preparedQuery->setString(2, std::to_string(pkt.charge));
        preparedQuery->execute();
    }
    
    
    
    cout << "Done." << endl;
    return EXIT_SUCCESS;
}

