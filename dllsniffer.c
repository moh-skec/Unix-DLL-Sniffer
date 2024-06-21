#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <net/if.h>
#include <stdio.h>
#define true 1
#define false 0
#define buffer_size 4096

void print_packet(const char *buffer, int len)
{
    for (int i = 0; i < len; ++i)
    {
        printf("%02x ", (unsigned char)buffer[i]);
        if ((i + 1) % 20 == 0)
        {
            printf("\n");
        }
    }
    printf("\033[0m \n****\n");
}
int find_lo_ifindex()
{
    int o1 = if_nametoindex("lo0");
    int o2 = if_nametoindex("lo");
    return o1 | o2;
}
int main()
{
    int sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1)
    {
        perror("\033[1;31mCan't create socket\n");
        printf("Did you forget to run as superuser???");
        printf("\033[0;0m\n");
        return 1;
    }
    struct sockaddr_ll phyaddr;
    char buffer[buffer_size];
    memset(buffer, 0, buffer_size);
    socklen_t len = sizeof(struct sockaddr_ll);

    while (true)
    {
        int recv_len = recvfrom(sockfd, buffer, buffer_size, 0, (struct sockaddr *)&phyaddr, &len);
        if (recv_len == -1)
        {
            perror("\033[1;31mThere where some error !!!\n");
            printf("\033[0;31m\n");
            usleep(500000);
            continue;
        }
        if (phyaddr.sll_ifindex == find_lo_ifindex()) // when both src and dst MAC address are all 0
        {                                             // differnet with PACKET_LOOPBACK
            printf("\033[1;31mLoopback packet !!!\n");
            printf("\033[0;31m");
        }
        if (phyaddr.sll_pkttype == PACKET_HOST)
        {
            printf("\033[1;32mIncoming:\n");
            printf("\033[0;32m");
        }
        else if (phyaddr.sll_pkttype == PACKET_BROADCAST)
        {
            printf("\033[1;34mBroadcast:\n");
            printf("\033[0;34m");
        }
        else if (phyaddr.sll_pkttype == PACKET_MULTICAST)
        {
            printf("\033[1;35mMulticast\n");
            printf("\033[0;35m");
        }
        else if (phyaddr.sll_pkttype == PACKET_OTHERHOST)
        {
            printf("\033[1;31mNot for us:\n");
            printf("\033[0;31m");
        }
        else if (phyaddr.sll_pkttype == PACKET_OUTGOING)
        {
            printf("\033[1;33mOutgoing:\n");
            printf("\033[0;33m");
        }
        else if (phyaddr.sll_pkttype == PACKET_LOOPBACK)
        {
            printf("Loopback\n");
        }
        else if (phyaddr.sll_pkttype == PACKET_FASTROUTE)
        {
            printf("Fastroute\n");
        }
        else
        {
            printf("\033[1;31mUnknown packet type !!!\n");
            printf("\033[0;31m");
        }
        print_packet(buffer, recv_len);
        memset(buffer, 0, buffer_size);
    }
    return 0;
}