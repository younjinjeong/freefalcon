/*
 * comenet.c
 *
 * ENet backend implementation for FreeFalcon COMAPI
 *
 * This provides reliable UDP networking using ENet library.
 * Replaces legacy DirectPlay with modern, cross-platform networking.
 *
 * Phase 3: Network Modernization
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "capi.h"

/*
 * ComApiHandle structure definition
 * Normally from capipriv.h, but we can't include that since it pulls in winsock.h
 * and ENet needs winsock2.h (included via windows.h)
 *
 * IMPORTANT: This must be defined BEFORE including comenet.h
 */
typedef struct ComApiHandle
{
    char *name;
    int protocol;
    int (*send_func)(struct ComApiHandle *c, int msgsize, int oob, int type);
    int (*send_dummy_func)(struct ComApiHandle *c, unsigned long ip, unsigned short port);
    int (*sendX_func)(struct ComApiHandle *c, int msgsize, int oob, int type, struct ComApiHandle *Xcom);
    int (*recv_func)(struct ComApiHandle *c);
    char * (*send_buf_func)(struct ComApiHandle *c);
    char * (*recv_buf_func)(struct ComApiHandle *c);
    int (*addr_func)(struct ComApiHandle *c, char *buf, int reset);
    void (*close_func)(struct ComApiHandle *c);
    unsigned long(*query_func)(struct ComApiHandle *c, int querytype);
    unsigned long(*get_timestamp_func)(struct ComApiHandle *c);
} ComAPI;

/* Now include comenet.h which uses ComApiHandle */
#include "comenet.h"

/* ENet initialization flag */
static int enet_initialized = 0;

/* Header constants */
#define ENET_HEADER_BASE 0xE7E7
#define ENET_HEADER_SIZE sizeof(ENetHeader)

/*=============================================================================
 * ENet Initialization
 *=============================================================================*/

int com_ENet_initialize(void)
{
    if (enet_initialized) {
        return 0; /* Already initialized */
    }

    if (enet_initialize() != 0) {
        fprintf(stderr, "ENet: Failed to initialize ENet library\n");
        return -1;
    }

    enet_initialized = 1;
    printf("ENet: Library initialized (version %d.%d.%d)\n",
           ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);

    return 0;
}

void com_ENet_shutdown(void)
{
    if (enet_initialized) {
        enet_deinitialize();
        enet_initialized = 0;
        printf("ENet: Library shutdown\n");
    }
}

const char* com_ENet_get_version(void)
{
    static char version[32];
    sprintf(version, "ENet %d.%d.%d", ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH);
    return version;
}

/*=============================================================================
 * Helper Functions
 *=============================================================================*/

static void com_ENet_init_header(ENetHeader* header, int size)
{
    header->header_base = ENET_HEADER_BASE;
    header->size = (unsigned short)size;
    header->inv_size = (unsigned short)(~size);
}

static int com_ENet_validate_header(ENetHeader* header)
{
    if (header->header_base != ENET_HEADER_BASE) {
        return 0; /* Invalid header */
    }
    if (header->size != (unsigned short)(~header->inv_size)) {
        return 0; /* Size mismatch */
    }
    return 1; /* Valid */
}

/*=============================================================================
 * COMAPI Function Pointer Wrappers
 * These adapt ENet functions to match COMAPI function pointer signatures
 *=============================================================================*/

/* Send function wrapper for COMAPI */
static int ComENetSend(struct ComApiHandle *c, int msgsize, int oob, int type)
{
    /* Note: oob and type parameters are ignored for ENet (always reliable) */
    ComENet* handle = (ComENet*)c;
    char* buffer = handle->send_buffer + ENET_HEADER_SIZE;
    return com_ENet_send((com_API_handle)c, buffer, msgsize);
}

/* Receive function wrapper for COMAPI */
static int ComENetRecv(struct ComApiHandle *c)
{
    ComENet* handle = (ComENet*)c;
    char* buffer = handle->recv_buffer + ENET_HEADER_SIZE;
    int max_len = handle->buffer_size;
    int received = com_ENet_recv((com_API_handle)c, buffer, max_len);

    if (received > 0) {
        handle->messagesize = received;
        return received;
    }
    return 0;
}

/* Send buffer get function wrapper for COMAPI */
static char* ComENetSendBufferGet(struct ComApiHandle *c)
{
    ComENet* handle = (ComENet*)c;
    /* Return pointer after header space */
    return handle->send_buffer + ENET_HEADER_SIZE;
}

/* Receive buffer get function wrapper for COMAPI */
static char* ComENetRecvBufferGet(struct ComApiHandle *c)
{
    ComENet* handle = (ComENet*)c;
    /* Return pointer after header space */
    return handle->recv_buffer + ENET_HEADER_SIZE;
}

/* Address function wrapper for COMAPI */
static int ComENetHostIDGet(struct ComApiHandle *c, char *buf, int reset)
{
    /* ENet doesn't expose raw addresses in the same way */
    /* This function is used for host ID - return 0 for now */
    return 0;
}

/* Close function wrapper for COMAPI */
static void ComENetClose(struct ComApiHandle *c)
{
    com_ENet_close((com_API_handle)c);
}

/* Query function wrapper for COMAPI */
static unsigned long ComENetQuery(struct ComApiHandle *c, int querytype)
{
    unsigned long result = 0;
    com_ENet_query((com_API_handle)c, querytype, &result);
    return result;
}

/* Timestamp function wrapper for COMAPI */
static unsigned long ComENetGetTimeStamp(struct ComApiHandle *c)
{
    ComENet* handle = (ComENet*)c;
    return handle->timestamp;
}

/* Dummy send function (not used by ENet) */
static int ComENetSendDummy(struct ComApiHandle *c, unsigned long ip, unsigned short port)
{
    /* Not applicable for ENet */
    return 0;
}

/* SendX function (not used by ENet) */
static int ComENetSendX(struct ComApiHandle *c, int msgsize, int oob, int type, struct ComApiHandle *Xcom)
{
    /* Not applicable for ENet - just use regular send */
    return ComENetSend(c, msgsize, oob, type);
}

/*=============================================================================
 * ENet Host (Server) Implementation
 *=============================================================================*/

com_API_handle com_ENet_open_host(int buffer_size, char* game_name,
                                   unsigned short port, int max_connections)
{
    ComENet* handle;
    ENetAddress address;

    /* Initialize ENet if needed */
    if (!enet_initialized) {
        if (com_ENet_initialize() != 0) {
            return NULL;
        }
    }

    /* Allocate handle */
    handle = (ComENet*)malloc(sizeof(ComENet));
    if (!handle) {
        fprintf(stderr, "ENet: Failed to allocate handle\n");
        return NULL;
    }
    memset(handle, 0, sizeof(ComENet));

    /* Initialize COMAPI header */
    handle->apiheader.protocol = CAPI_ENET_PROTOCOL;
    handle->apiheader.send_func = ComENetSend;
    handle->apiheader.send_dummy_func = ComENetSendDummy;
    handle->apiheader.sendX_func = ComENetSendX;
    handle->apiheader.recv_func = ComENetRecv;
    handle->apiheader.send_buf_func = ComENetSendBufferGet;
    handle->apiheader.recv_buf_func = ComENetRecvBufferGet;
    handle->apiheader.addr_func = ComENetHostIDGet;
    handle->apiheader.close_func = ComENetClose;
    handle->apiheader.query_func = ComENetQuery;
    handle->apiheader.get_timestamp_func = ComENetGetTimeStamp;

    /* Initialize handle */
    handle->buffer_size = buffer_size;
    handle->handletype = 1; /* Host */
    handle->port = port;
    handle->max_peers = (max_connections > 32) ? 32 : max_connections;
    handle->headersize = ENET_HEADER_SIZE;
    handle->state = COMAPI_STATE_CONNECTED;

    /* Allocate buffers */
    handle->send_buffer = (char*)malloc(buffer_size + ENET_HEADER_SIZE);
    handle->recv_buffer_start = (char*)malloc(buffer_size + ENET_HEADER_SIZE);
    handle->recv_buffer = handle->recv_buffer_start;

    if (!handle->send_buffer || !handle->recv_buffer_start) {
        fprintf(stderr, "ENet: Failed to allocate buffers\n");
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }

    handle->Header = (ENetHeader*)handle->recv_buffer_start;

    /* Create ENet host */
    address.host = ENET_HOST_ANY;
    address.port = port;

    handle->host = enet_host_create(&address,
                                     handle->max_peers,  /* max clients */
                                     2,                  /* 2 channels */
                                     0,                  /* no bandwidth limit */
                                     0);                 /* no bandwidth limit */

    if (!handle->host) {
        fprintf(stderr, "ENet: Failed to create host on port %d\n", port);
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }

    /* Create lock */
    handle->lock = CreateMutex(NULL, FALSE, NULL);

    printf("ENet: Host created on port %d (max %d connections)\n", port, handle->max_peers);

    return (com_API_handle)handle;
}

/*=============================================================================
 * ENet Client Implementation
 *=============================================================================*/

com_API_handle com_ENet_open_client(int buffer_size, char* address,
                                     unsigned short port)
{
    ComENet* handle;
    ENetAddress enet_address;
    ENetEvent event;

    /* Initialize ENet if needed */
    if (!enet_initialized) {
        if (com_ENet_initialize() != 0) {
            return NULL;
        }
    }

    /* Allocate handle */
    handle = (ComENet*)malloc(sizeof(ComENet));
    if (!handle) {
        fprintf(stderr, "ENet: Failed to allocate handle\n");
        return NULL;
    }
    memset(handle, 0, sizeof(ComENet));

    /* Initialize COMAPI header */
    handle->apiheader.protocol = CAPI_ENET_PROTOCOL;
    handle->apiheader.send_func = ComENetSend;
    handle->apiheader.send_dummy_func = ComENetSendDummy;
    handle->apiheader.sendX_func = ComENetSendX;
    handle->apiheader.recv_func = ComENetRecv;
    handle->apiheader.send_buf_func = ComENetSendBufferGet;
    handle->apiheader.recv_buf_func = ComENetRecvBufferGet;
    handle->apiheader.addr_func = ComENetHostIDGet;
    handle->apiheader.close_func = ComENetClose;
    handle->apiheader.query_func = ComENetQuery;
    handle->apiheader.get_timestamp_func = ComENetGetTimeStamp;

    /* Initialize handle */
    handle->buffer_size = buffer_size;
    handle->handletype = 0; /* Client */
    handle->port = port;
    strncpy(handle->address, address, sizeof(handle->address) - 1);
    handle->headersize = ENET_HEADER_SIZE;
    handle->state = COMAPI_STATE_CONNECTION_PENDING;

    /* Allocate buffers */
    handle->send_buffer = (char*)malloc(buffer_size + ENET_HEADER_SIZE);
    handle->recv_buffer_start = (char*)malloc(buffer_size + ENET_HEADER_SIZE);
    handle->recv_buffer = handle->recv_buffer_start;

    if (!handle->send_buffer || !handle->recv_buffer_start) {
        fprintf(stderr, "ENet: Failed to allocate buffers\n");
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }

    handle->Header = (ENetHeader*)handle->recv_buffer_start;

    /* Create ENet client */
    handle->host = enet_host_create(NULL,              /* no address (client) */
                                     1,                 /* 1 outgoing connection */
                                     2,                 /* 2 channels */
                                     0,                 /* no bandwidth limit */
                                     0);                /* no bandwidth limit */

    if (!handle->host) {
        fprintf(stderr, "ENet: Failed to create client\n");
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }

    /* Resolve address */
    if (enet_address_set_host(&enet_address, address) != 0) {
        fprintf(stderr, "ENet: Failed to resolve address: %s\n", address);
        enet_host_destroy(handle->host);
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }
    enet_address.port = port;

    /* Connect to server */
    handle->peer = enet_host_connect(handle->host, &enet_address, 2, 0);
    if (!handle->peer) {
        fprintf(stderr, "ENet: Failed to initiate connection\n");
        enet_host_destroy(handle->host);
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }

    /* Wait for connection (5 second timeout) */
    if (enet_host_service(handle->host, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("ENet: Connected to %s:%d\n", address, port);
        handle->state = COMAPI_STATE_CONNECTED;
    } else {
        fprintf(stderr, "ENet: Connection timeout\n");
        enet_peer_reset(handle->peer);
        enet_host_destroy(handle->host);
        free(handle->send_buffer);
        free(handle->recv_buffer_start);
        free(handle);
        return NULL;
    }

    /* Create lock */
    handle->lock = CreateMutex(NULL, FALSE, NULL);

    return (com_API_handle)handle;
}

/*=============================================================================
 * Send/Receive Implementation
 *=============================================================================*/

int com_ENet_send(com_API_handle c, char* buffer, int len)
{
    ComENet* handle = (ComENet*)c;
    ENetPacket* packet;
    char* packet_data;
    int total_size;
    int result = 0;

    if (!handle || !handle->host) {
        return COMAPI_BAD_HEADER;
    }

    WaitForSingleObject(handle->lock, INFINITE);

    /* Prepare packet with header */
    total_size = len + ENET_HEADER_SIZE;
    packet_data = (char*)malloc(total_size);
    if (!packet_data) {
        ReleaseMutex(handle->lock);
        return COMAPI_MESSAGE_TOO_BIG;
    }

    /* Write header */
    com_ENet_init_header((ENetHeader*)packet_data, len);

    /* Copy payload */
    memcpy(packet_data + ENET_HEADER_SIZE, buffer, len);

    /* Create ENet packet (reliable, channel 0) */
    packet = enet_packet_create(packet_data, total_size, ENET_PACKET_FLAG_RELIABLE);
    free(packet_data);

    if (!packet) {
        ReleaseMutex(handle->lock);
        return COMAPI_MESSAGE_TOO_BIG;
    }

    /* Send to appropriate peer(s) */
    if (handle->handletype == 0) {
        /* Client: send to server */
        if (handle->peer && handle->peer->state == ENET_PEER_STATE_CONNECTED) {
            enet_peer_send(handle->peer, 0, packet);
            result = len;
            handle->sendmessagecount++;
        } else {
            enet_packet_destroy(packet);
            result = COMAPI_CONNECTION_CLOSED;
        }
    } else {
        /* Host: broadcast to all connected peers */
        enet_host_broadcast(handle->host, 0, packet);
        result = len;
        handle->sendmessagecount++;
    }

    /* Flush send queue */
    enet_host_flush(handle->host);

    ReleaseMutex(handle->lock);
    return result;
}

int com_ENet_recv(com_API_handle c, char* buffer, int len)
{
    ComENet* handle = (ComENet*)c;
    ENetEvent event;
    int result = 0;

    if (!handle || !handle->host) {
        return COMAPI_BAD_HEADER;
    }

    WaitForSingleObject(handle->lock, INFINITE);

    /* Service the host (non-blocking) */
    while (enet_host_service(handle->host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            printf("ENet: Peer connected from %x:%u\n",
                   event.peer->address.host, event.peer->address.port);

            /* Add to peer list (host mode) */
            if (handle->handletype == 1 && handle->peer_count < handle->max_peers) {
                handle->peers[handle->peer_count++] = event.peer;
            }

            if (handle->accept_callback_func) {
                handle->accept_callback_func((struct ComApiHandle*)handle, 0);
            }
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            /* Validate header */
            if (event.packet->dataLength < ENET_HEADER_SIZE) {
                enet_packet_destroy(event.packet);
                break;
            }

            ENetHeader* header = (ENetHeader*)event.packet->data;
            if (!com_ENet_validate_header(header)) {
                fprintf(stderr, "ENet: Invalid packet header\n");
                enet_packet_destroy(event.packet);
                break;
            }

            /* Copy payload to buffer */
            int payload_size = event.packet->dataLength - ENET_HEADER_SIZE;
            if (payload_size <= len) {
                memcpy(buffer, event.packet->data + ENET_HEADER_SIZE, payload_size);
                result = payload_size;
                handle->recvmessagecount++;
            } else {
                result = COMAPI_MESSAGE_TOO_BIG;
            }

            enet_packet_destroy(event.packet);
            ReleaseMutex(handle->lock);
            return result;

        case ENET_EVENT_TYPE_DISCONNECT:
            printf("ENet: Peer disconnected\n");

            /* Remove from peer list */
            if (handle->handletype == 1) {
                for (int i = 0; i < handle->peer_count; i++) {
                    if (handle->peers[i] == event.peer) {
                        /* Shift remaining peers */
                        for (int j = i; j < handle->peer_count - 1; j++) {
                            handle->peers[j] = handle->peers[j + 1];
                        }
                        handle->peer_count--;
                        break;
                    }
                }
            }

            event.peer->data = NULL;
            break;

        case ENET_EVENT_TYPE_NONE:
            break;
        }
    }

    ReleaseMutex(handle->lock);
    return result; /* No data available */
}

/*=============================================================================
 * Close Implementation
 *=============================================================================*/

void com_ENet_close(com_API_handle c)
{
    ComENet* handle = (ComENet*)c;
    ENetEvent event;

    if (!handle) {
        return;
    }

    WaitForSingleObject(handle->lock, INFINITE);

    /* Disconnect peer(s) */
    if (handle->handletype == 0 && handle->peer) {
        enet_peer_disconnect(handle->peer, 0);

        /* Wait for disconnect acknowledgment */
        while (enet_host_service(handle->host, &event, 3000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
    }

    /* Destroy host */
    if (handle->host) {
        enet_host_destroy(handle->host);
    }

    /* Free buffers */
    free(handle->send_buffer);
    free(handle->recv_buffer_start);

    ReleaseMutex(handle->lock);
    CloseHandle(handle->lock);

    printf("ENet: Connection closed\n");
    free(handle);
}

/*=============================================================================
 * Query Implementation
 *=============================================================================*/

int com_ENet_query(com_API_handle c, int query_type, void* result)
{
    ComENet* handle = (ComENet*)c;

    if (!handle) {
        return -1;
    }

    switch (query_type) {
    case COMAPI_PROTOCOL:
        *(int*)result = CAPI_ENET_PROTOCOL;
        return 0;

    case COMAPI_STATE:
        *(int*)result = handle->state;
        return 0;

    case COMAPI_SEND_MESSAGECOUNT:
        *(unsigned long*)result = handle->sendmessagecount;
        return 0;

    case COMAPI_RECV_MESSAGECOUNT:
        *(unsigned long*)result = handle->recvmessagecount;
        return 0;

    case COMAPI_MAX_BUFFER_SIZE:
        *(int*)result = handle->buffer_size;
        return 0;

    case COMAPI_ACTUAL_BUFFER_SIZE:
        *(int*)result = handle->buffer_size;
        return 0;

    default:
        return -1;
    }
}
