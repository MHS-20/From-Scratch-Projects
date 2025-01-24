#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <pthread.h>

struct vport_t
{
    int tapfd;                       // TAP device FD
    int vport_sockfd;                // client socket
    struct sockaddr_in vswitch_addr; // vSwitch address
};