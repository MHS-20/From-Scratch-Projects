#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_FRAME_SIZE 1518
#define BROADCAST_MAC "ff:ff:ff:ff:ff:ff"

// Struct storing MAC-vPort mapping (mac table)
struct mac_entry
{
    char mac_addr[18];
    struct sockaddr_in vport_addr;
};

// Simple MAC address table
#define MAX_MAC_ENTRIES 256
struct mac_entry mac_table[MAX_MAC_ENTRIES];
int mac_table_size = 0;

// Function to add or update the MAC table
void update_mac_table(const char *mac_addr, struct sockaddr_in *vport_addr)
{
    for (int i = 0; i < mac_table_size; i++)
    {
        if (strcmp(mac_table[i].mac_addr, mac_addr) == 0)
        {
            mac_table[i].vport_addr = *vport_addr;
            return;
        }
    }

    // Add a new entry
    if (mac_table_size < MAX_MAC_ENTRIES)
    {
        strcpy(mac_table[mac_table_size].mac_addr, mac_addr);
        mac_table[mac_table_size].vport_addr = *vport_addr;
        mac_table_size++;
    }
    else
    {
        fprintf(stderr, "MAC table full, cannot add new entry\n");
    }
}

// Function to find a MAC address in the table
struct sockaddr_in *find_mac_entry(const char *mac_addr)
{
    for (int i = 0; i < mac_table_size; i++)
    {
        if (strcmp(mac_table[i].mac_addr, mac_addr) == 0)
        {
            return &mac_table[i].vport_addr;
        }
    }
    return NULL;
}

// Helper function to convert MAC address to a readable string
void mac_to_str(const unsigned char *mac, char *buf)
{
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void print_mac_table()
{
    printf("    ARP Cache: {");
    for (int i = 0; i < mac_table_size; i++)
    {
        printf("%s: %s", mac_table[i].mac_addr, inet_ntoa(mac_table[i].vport_addr.sin_addr));
        if (i < mac_table_size - 1)
        {
            printf(", ");
        }
    }
    printf("}\n");
}

// ----- MAIN ----
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s {VSWITCH_PORT}\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_port = atoi(argv[1]);
    struct sockaddr_in server_addr;

    // Create UDP socket
    int vserver_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (vserver_sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to the server port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    if (bind(vserver_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(vserver_sock);
        exit(EXIT_FAILURE);
    }

    printf("[VSwitch] Started at 0.0.0.0:%d\n", server_port);

    char ether_data[MAX_FRAME_SIZE];
    struct sockaddr_in vport_addr;
    socklen_t addr_len = sizeof(vport_addr);

    while (1)
    {
        // Read Ethernet frame from VPort
        int recv_len = recvfrom(vserver_sock, ether_data, sizeof(ether_data), 0,
                                (struct sockaddr *)&vport_addr, &addr_len);
        if (recv_len <= 0)
        {
            perror("recvfrom");
            continue;
        }

        // Parse Ethernet frame
        char eth_dst[18], eth_src[18];
        mac_to_str((unsigned char *)ether_data, eth_dst);
        mac_to_str((unsigned char *)(ether_data + 6), eth_src);

        printf("[VSwitch] vport_addr<%s:%d> src<%s> dst<%s> datasz<%d>\n",
               inet_ntoa(vport_addr.sin_addr), ntohs(vport_addr.sin_port),
               eth_src, eth_dst, recv_len);

        // Update MAC table
        update_mac_table(eth_src, &vport_addr);

        // Forward Ethernet frame
        if (strcmp(eth_dst, BROADCAST_MAC) == 0)
        {
            printf("    Broadcast to all VPorts except source\n");
            for (int i = 0; i < mac_table_size; i++)
            {
                if (strcmp(mac_table[i].mac_addr, eth_src) != 0)
                {
                    sendto(vserver_sock, ether_data, recv_len, 0,
                           (struct sockaddr *)&mac_table[i].vport_addr,
                           sizeof(mac_table[i].vport_addr));
                }
            }
        }
        else
        {
            struct sockaddr_in *dst_addr = find_mac_entry(eth_dst);
            if (dst_addr)
            {
                sendto(vserver_sock, ether_data, recv_len, 0,
                       (struct sockaddr *)dst_addr, sizeof(*dst_addr));
                printf("    Forwarded to: %s\n", eth_dst);
            }
            else
            {
                printf("    Discarded\n");
            }
        }
    }

    close(vserver_sock);
    return 0;
}
