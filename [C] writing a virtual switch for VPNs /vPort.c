#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <pthread.h>
#include "tap_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define ERROR_PRINT_THEN_EXIT(msg...) \
    fprintf(stderr, ##msg);           \
    exit(1);

struct vport_t
{
    int tapfd;                       // TAP device FD
    int vport_sockfd;                // client socket
    struct sockaddr_in vswitch_addr; // vSwitch address
};

void vport_init(struct vport_t *vport, const char *server_ip_str, int server_port);

// reads data from the TAP device and forwards it to the vSwitch
void *forward_ethernet_data_to_vswitch(void *raw_vport);

// reads data from the vSwitch and forwards it to the TAP device.
void *forward_ethernet_data_to_tap(void *raw_vport);

int main(int argc, char const *argv[])
{
    // parse arguments
    if (argc != 3)
    {
        ERROR_PRINT_THEN_EXIT("Usage: vport {server_ip} {server_port}\n");
    }
    const char *server_ip_str = argv[1];
    int server_port = atoi(argv[2]);

    // vport init
    struct vport_t vport;
    vport_init(&vport, server_ip_str, server_port);

    // up forwarder
    pthread_t up_forwarder;
    if (pthread_create(&up_forwarder, NULL, forward_ethernet_data_to_vswitch, &vport) != 0)
    {
        ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
    }

    // down forwarder
    pthread_t down_forwarder;
    if (pthread_create(&down_forwarder, NULL, forward_ethernet_data_to_tap, &vport) != 0)
    {
        ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
    }

    // wait for up forwarder & down forwarder
    if (pthread_join(up_forwarder, NULL) != 0 || pthread_join(down_forwarder, NULL) != 0)
    {
        ERROR_PRINT_THEN_EXIT("fail to pthread_join: %s\n", strerror(errno));
    }

    return 0;
}

/**
 * init VPort
 * create TAP device and client socket
 */
void vport_init(struct vport_t *vport, const char *server_ip_str, int server_port)
{
    // alloc tap device
    char ifname[IFNAMSIZ] = "tapyuan";
    int tapfd = tap_alloc(ifname);
    if (tapfd < 0)
    {
        ERROR_PRINT_THEN_EXIT("fail to tap_alloc: %s\n", strerror(errno));
    }

    // create socket
    int vport_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (vport_sockfd < 0)
    {
        ERROR_PRINT_THEN_EXIT("fail to socket: %s\n", strerror(errno));
    }

    // setup vSwitch info
    struct sockaddr_in vswitch_addr;
    memset(&vswitch_addr, 0, sizeof(vswitch_addr));
    vswitch_addr.sin_family = AF_INET;
    vswitch_addr.sin_port = htons(server_port); // to big endian
    if (inet_pton(AF_INET, server_ip_str, &vswitch_addr.sin_addr) != 1) // convert IP string to bytes
    {
        ERROR_PRINT_THEN_EXIT("fail to inet_pton: %s\n", strerror(errno));
    }

    vport->tapfd = tapfd;
    vport->vport_sockfd = vport_sockfd;
    vport->vswitch_addr = vswitch_addr;

    printf("[VPort] TAP device name: %s, VSwitch: %s:%d\n", ifname, server_ip_str, server_port);
}