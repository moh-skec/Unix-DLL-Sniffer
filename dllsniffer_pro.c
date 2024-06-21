#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
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

void DisplayFrameInfo(const char *buffer)
{
    struct ethhdr *ethh = (struct ethhdr *)buffer;
    uint16_t packet_type = ntohs(ethh->h_proto);

    // print MAC Address
    printf("src MAC: ");
    for (int i = 0; i < 5; ++i)
        printf("%02x:", ethh->h_source[i]);
    printf("%02x", ethh->h_source[5]);
    printf(", dst MAC: ");
    for (int i = 0; i < 5; ++i)
        printf("%02x:", ethh->h_dest[i]);
    printf("%02x", ethh->h_dest[5]);

    printf("\nUpper protocol: ");
    if (packet_type == ETH_P_ARP)
    {
        printf("ARP\n");
    }
    else if (packet_type == ETH_P_IP)
    {
        printf("IPv4\n");
    }
    else if (packet_type == ETH_P_IPV6)
    {
        printf("IPv6\n");
    }
    else
    {
        printf("Unknown protocol %04x", packet_type);
    }
}
int find_lo_ifindex()
{
    int o1 = if_nametoindex("lo0");
    int o2 = if_nametoindex("lo");
    return o1 | o2;
}
void print_IPv6_packet(const char *buffer)
{
    struct ip6_hdr *pkt = (struct ip6_hdr *)(buffer + sizeof(struct ethhdr));
    char src_addr[INET6_ADDRSTRLEN];
    char dst_addr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(pkt->ip6_src), src_addr, INET6_ADDRSTRLEN);
    inet_ntop(AF_INET6, &(pkt->ip6_dst), dst_addr, INET6_ADDRSTRLEN);
    printf("Payload len: %d \n", ntohs(pkt->ip6_plen));
    printf("src IP: %s\n", src_addr);
    printf("dst IP: %s\n", dst_addr);
}
void print_IPv4_packet(const char *buffer)
{
#ifdef __USE_MISC
    struct ip *pkt = (struct ip *)(buffer + sizeof(struct ethhdr));
    printf("header len: %d, total len: %d, proto: %d\n", pkt->ip_hl * 4, ntohs(pkt->ip_len), pkt->ip_p);
    printf("src IP:%s, ", inet_ntoa(pkt->ip_src));
    printf("dst IP:%s\n", inet_ntoa(pkt->ip_dst));
#else
    struct iphdr *pkt = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    printf("header len: %d, total len: %d, proto: %d\n", pkt->ihl * 4, ntohs(pkt->tot_len), pkt->protocol);
    struct in_addr src_addr, dest_addr;
    src_addr.s_addr = pkt->saddr;
    dest_addr.s_addr = pkt->daddr;
    printf("src IP:%s, dst IP:%s \n", inet_ntoa(src_addr), inet_ntoa(dest_addr));
#endif
}
void print_IP_packet(const char *buffer)
{
    struct ethhdr *ethh = (struct ethhdr *)buffer;
    if (ntohs(ethh->h_proto) == ETH_P_IP)
        print_IPv4_packet(buffer);
    else if (ntohs(ethh->h_proto) == ETH_P_IPV6)
        print_IPv6_packet(buffer);
    else
        return;
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
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
            printf("\033[0;32m");
        }
        else if (phyaddr.sll_pkttype == PACKET_BROADCAST)
        {
            printf("\033[1;34mBroadcast:\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
            printf("\033[0;34m");
        }
        else if (phyaddr.sll_pkttype == PACKET_MULTICAST)
        {
            printf("\033[1;35mMulticast\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
            printf("\033[0;35m");
        }
        else if (phyaddr.sll_pkttype == PACKET_OTHERHOST)
        {
            printf("\033[1;31mNot for us:\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
            printf("\033[0;31m");
        }
        else if (phyaddr.sll_pkttype == PACKET_OUTGOING)
        {
            printf("\033[1;33mOutgoing:\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
            printf("\033[0;33m");
        }
        else if (phyaddr.sll_pkttype == PACKET_LOOPBACK)
        {
            printf("Loopback\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
        }
        else if (phyaddr.sll_pkttype == PACKET_FASTROUTE)
        {
            printf("Fastroute\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
        }
        else
        {
            printf("\033[1;31mUnknown packet type !!!\n");
            DisplayFrameInfo(buffer);
            print_IP_packet(buffer);
        }
        print_packet(buffer, recv_len);
        memset(buffer, 0, buffer_size);
    }
    return 0;
}