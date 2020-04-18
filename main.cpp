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


// Check Sum 
unsigned short checksum(void *b, int len) 
{    
    unsigned short *buf = (unsigned short*) b; 
    unsigned int sum=0; 
    unsigned short result; 
  
    for ( sum = 0; len > 1; len -= 2 ) 
        sum += *buf++; 
    if ( len == 1 ) 
        sum += *(unsigned char*)buf; 
    sum = (sum >> 16) + (sum & 0xFFFF); 
    sum += (sum >> 16); 
    result = ~sum; 

    return result; 
} 

void ping(struct sockaddr *dst) {

    int socketfd;
    int socketOpt;
    int ttlValue = 64;
    int msgCount = 0;
    long double rttMSec = 0;
    bool pktSent; 
    char* ipAddr;
    struct pingPkt pkt;
    struct sockaddr recv;
    struct timespec time_start, time_end;

    struct timeval timeOut;
    timeOut.tv_sec = 1;
    timeOut.tv_usec = 0;

    // Create raw socket and configuring it to use TTL in the IP header
    if (dst->sa_family == AF_INET) {
        ipAddr = (char*) malloc(INET_ADDRSTRLEN);
        struct sockaddr_in * hostIP4 = (sockaddr_in *) dst;
        inet_ntop(AF_INET, &(hostIP4->sin_addr), ipAddr, INET_ADDRSTRLEN);
        // Configure socket to use TTL in IP header and to timeout on receiving
        socketfd =  socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        socketOpt = setsockopt(socketfd, IPPROTO_IP, IP_TTL, &ttlValue, sizeof(ttlValue));
    } else if (dst->sa_family == AF_INET6) {
        ipAddr = (char*) malloc(INET6_ADDRSTRLEN);
        struct sockaddr_in6* hostIP6 = (sockaddr_in6*) dst;
        inet_ntop(AF_INET6, &(hostIP6->sin6_addr), ipAddr, INET6_ADDRSTRLEN);
        socketfd =  socket(AF_INET6, SOCK_RAW, IPPROTO_ICMP);
        socketOpt = setsockopt(socketfd, IPPROTO_IPV6, IP_TTL, &ttlValue, sizeof(ttlValue));
    }

    if (socketfd == -1) {
        cout << "Error when creating socket descriptor. \n";
        return;
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

        // Ping once every second
        usleep(1000000);
        
        // Send packet
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        if (sendto(socketfd, &pkt, sizeof(pkt), 0, dst, sizeof(*dst)) == -1) {
            cout << "Packet failed to send.\n";
            pktSent = false;
        } 
        else { 
            pktSent = true;
        }
        
        unsigned int recv_len = sizeof(recv);

        // Receive packet
        if (recvfrom(socketfd, &pkt, sizeof(pkt), 0, &recv, &recv_len) == -1) {
            cout << "Receive packet function failed.\n";
        } else {

            // Calculate RTT
            clock_gettime(CLOCK_MONOTONIC, &time_end);
            rttMSec = (double) (time_end.tv_nsec - time_start.tv_nsec) / 1000000;

            // Don't receive packet if not sent
            if (pktSent) {
                if (pkt.icmp.icmp_code == 0) {

                    // Output packet stat
                    cout << "64 bytes from " << ipAddr << ": icmp_seq= " << msgCount << " ttl=" << ttlValue << 
                        " rtt=" << rttMSec << " ms.\n";
                } else {
                    cout << "Error: Packet received with ICMP type: " << pkt.icmp.icmp_type << 
                        " and code: " << pkt.icmp.icmp_code << '\n';
                }
            }


        }
        
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