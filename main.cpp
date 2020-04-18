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

    // Get raw socket descriptor
    int socketfd =  socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socketfd == -1) {
        cout << "Error when creating socket descriptor. \n";
        return;
    } 

    int ttlValue = 64;
    int msgCount = 0;
    long rttMSec = 0;
    bool pktSent; 
    struct pingPkt pkt;
    struct sockaddr recv;
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

            clock_gettime(CLOCK_MONOTONIC, &time_end);
            rttMSec = (time_end.tv_sec - time_start.tv_sec) * 1000.0;

            // Don't receive packet if not sent
            if (pktSent) {
                if (pkt.icmp.icmp_code == 0) {
                    char* hostname;
                    if (dst->sa_family == AF_INET) {
                        struct sockaddr_in * hostIP4 = (sockaddr_in *) dst;
                        inet_ntop(AF_INET, &(hostIP4->sin_addr), hostname, INET_ADDRSTRLEN);
                    } else if (dst->sa_family == AF_INET6) {
                        struct sockaddr_in6* hostIP6 = (sockaddr_in6*) dst;
                        inet_ntop(AF_INET6, &(hostIP6->sin6_addr), hostname, INET6_ADDRSTRLEN);
                    }

                    cout << "\n\n\nReceived packet! :) \n";
                    cout << time_start.tv_sec << '\n';
                    cout << time_end.tv_sec << '\n';
                    cout << "Received packet sandwhich! :) \n\n\n";


                    cout << "64 bytes from " << hostname << ": icmp_seq= " << msgCount << " ttl=" << ttlValue << 
                        " time=" << rttMSec << " ms.\n";
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