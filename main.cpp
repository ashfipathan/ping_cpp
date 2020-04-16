#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <iostream>

using namespace std;


void ping(struct sockaddr *dst) {
 cout << "Made it to the ping function \n";
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: ./ping <address>\n");
        return 1;
    }
    struct addrinfo *infoptr;

    if (getaddrinfo(argv[1], nullptr, nullptr, &infoptr)) {
        cout << "Unable to connect to given address. \n";
        return 1;
    }

    ping(infoptr->ai_addr);
    return 0;
}