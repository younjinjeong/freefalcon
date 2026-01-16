#ifndef COMENET_H
#define COMENET_H

/*
 * comenet.h
 *
 * ENet backend for FreeFalcon COMAPI
 *
 * This provides a modern, cross-platform replacement for DirectPlay
 * using the ENet library (reliable UDP with sequencing and channels).
 *
 * Phase 3: Network Modernization
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <enet/enet.h>

/* ENet packet header for message framing */
typedef struct enetheader
{
    unsigned short header_base;
    unsigned short size;
    unsigned short inv_size;
} ENetHeader;

/* ENet COMAPI handle structure */
typedef struct comenethandle
{
    struct ComApiHandle apiheader;

    /* ENet objects */
    ENetHost* host;                    /* ENet host (server or client) */
    ENetPeer* peer;                    /* Connected peer (client mode) or NULL (server mode) */
    ENetPeer* peers[32];               /* Array of connected peers (server mode) */
    int peer_count;                    /* Number of connected peers */
    int max_peers;                     /* Maximum number of peers */

    /* Buffer management */
    int buffer_size;
    char* send_buffer;
    char* recv_buffer_start;
    char* recv_buffer;

    /* Statistics */
    unsigned long sendmessagecount;
    unsigned long recvmessagecount;
    unsigned long sendwouldblockcount;
    unsigned long recvwouldblockcount;

    /* State management */
    HANDLE lock;
    HANDLE ThreadHandle;
    short ThreadActive;
    short timeoutsecs;
    short state;
    short handletype;                  /* 0=client, 1=host */
    int referencecount;

    /* Message handling */
    int messagesize;
    int headersize;
    int bytes_needed_for_header;
    long bytes_needed_for_message;
    int bytes_recvd_for_message;
    ENetHeader* Header;

    /* Connection info */
    char address[64];                  /* IP address or hostname */
    unsigned short port;               /* Port number */

    /* Callbacks */
    void (*connect_callback_func)(struct ComApiHandle* c, int retcode);
    void (*accept_callback_func)(struct ComApiHandle* c, int retcode);

    unsigned long timestamp;
} ComENet;

/* ENet COMAPI function declarations */

/* Initialize ENet library (call once at startup) */
int com_ENet_initialize(void);

/* Shutdown ENet library (call once at exit) */
void com_ENet_shutdown(void);

/* Open ENet host (server mode) */
com_API_handle com_ENet_open_host(int buffer_size, char* game_name,
                                   unsigned short port, int max_connections);

/* Open ENet client connection */
com_API_handle com_ENet_open_client(int buffer_size, char* address,
                                     unsigned short port);

/* Send data */
int com_ENet_send(com_API_handle c, char* buffer, int len);

/* Receive data */
int com_ENet_recv(com_API_handle c, char* buffer, int len);

/* Close connection */
void com_ENet_close(com_API_handle c);

/* Query ENet handle properties */
int com_ENet_query(com_API_handle c, int query_type, void* result);

/* Get ENet version string */
const char* com_ENet_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* COMENET_H */
