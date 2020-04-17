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
#include <unistd.h>

using namespace std;

// Ping packet with a size of 64
struct pingPkt 
{ 
    struct icmp icmp; 
    char msg[64 - sizeof(icmp)]; 
}; 


unsigned short checksum(void *b, int len) {
}

void ping(struct sockaddr *dst) {

    // Get raw socket descriptor
    int socketfd =  socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socketfd == -1) {
        cout << "Error when creating socket descriptor. \n";
        return;
    } 

    int ttlValue = 64;
    int msgCount = 0;
    long rttMSec = 0;
    
    struct pingPkt pkt;
    struct timespec time_start, time_end, tfs, tfe;
    struct timeval timeOut;
    timeOut.tv_sec = 1;
    timeOut.tv_usec = 0;

    // Configuring socket to use TTL in the IP header
    int socketOpt = setsockopt(socketfd, IPPROTO_IP, IP_TTL, &ttlValue, sizeof(ttlValue));
    if (socketOpt == -1) {
        cout << "Unable to configure socket. \n";
    }

    // Configure socket to timeout on receiving
    setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,(const char*) &timeOut, sizeof(timeOut));

    // Continuously send icmp packets
    while (1)
    {
        // empty packet
        bzero(&pkt, sizeof(pkt));
        pkt.icmp.icmp_type = ICMP_ECHO;
        pkt.icmp.icmp_hun.ih_idseq.icd_id = getpid();
        // Fill up the message in the packet
        for (int i = 0; i < sizeof(pkt.msg) - 1; i++) {
            pkt.msg[i] = i + '0';
        }
        pkt.icmp.icmp_hun.ih_idseq.icd_seq = msgCount++;
        pkt.icmp.icmp_cksum = checksum(&pkt, sizeof(pkt));

    }
    

}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        cout << "usage: ./ping <address> \n";
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