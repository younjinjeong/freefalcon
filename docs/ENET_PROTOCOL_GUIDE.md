# ENet Protocol Guide for FreeFalcon

## Overview

FreeFalcon now supports the **ENet protocol** for multiplayer networking. ENet provides modern, reliable UDP networking with better performance than legacy DirectPlay or TCP protocols.

**Version:** FreeFalcon with ENet 1.3.18
**Protocol ID:** `CAPI_ENET_PROTOCOL` (12)
**Status:** Production Ready

## What is ENet?

ENet is a reliable UDP networking library designed for real-time applications like games. It provides:

- ✅ **Reliable delivery** - Guaranteed packet delivery with automatic retransmission
- ✅ **Low latency** - UDP-based for minimal overhead
- ✅ **Sequencing** - Packets arrive in order
- ✅ **Fragmentation** - Automatic handling of large messages
- ✅ **Cross-platform** - Works on Windows, Linux, macOS
- ✅ **Battle-tested** - Used in many commercial games

## Quick Start

### For Server (Host)

```c
#include "comms/capi.h"

// 1. Initialize ENet
com_ENet_initialize();

// 2. Create server (host)
com_API_handle host = com_ENet_open_host(
    8192,           // Buffer size (bytes)
    "FreeFalcon",   // Game name
    2934,           // Port number
    16              // Max clients (1-32)
);

if (host == NULL) {
    fprintf(stderr, "Failed to create ENet host\n");
    return -1;
}

// 3. Send data to all connected clients
char* send_buffer = ComAPISendBufferGet(host);
memcpy(send_buffer, game_data, data_size);
ComAPISend(host, data_size, 0);

// 4. Receive data from clients
char* recv_buffer = ComAPIRecvBufferGet(host);
int received = com_ENet_recv(host, recv_buffer, 8192);
if (received > 0) {
    // Process received data
    process_client_data(recv_buffer, received);
}

// 5. Close when done
ComAPIClose(host);
com_ENet_shutdown();
```

### For Client

```c
#include "comms/capi.h"

// 1. Initialize ENet
com_ENet_initialize();

// 2. Connect to server
com_API_handle client = com_ENet_open_client(
    8192,              // Buffer size (bytes)
    "192.168.1.100",   // Server IP address
    2934               // Server port
);

if (client == NULL) {
    fprintf(stderr, "Failed to connect to server\n");
    return -1;
}

// 3. Check connection state
unsigned long state = ComAPIQuery(client, COMAPI_STATE);
if (state == COMAPI_STATE_CONNECTED) {
    printf("Connected to server!\n");
}

// 4. Send data to server
char* send_buffer = ComAPISendBufferGet(client);
memcpy(send_buffer, player_data, data_size);
ComAPISend(client, data_size, 0);

// 5. Receive data from server
char* recv_buffer = ComAPIRecvBufferGet(client);
int received = com_ENet_recv(client, recv_buffer, 8192);
if (received > 0) {
    // Process received data
    process_server_data(recv_buffer, received);
}

// 6. Close when done
ComAPIClose(client);
com_ENet_shutdown();
```

## API Reference

### Initialization Functions

#### `com_ENet_initialize()`
Initializes the ENet library. Call once at program startup.

**Returns:** 0 on success, -1 on failure

**Example:**
```c
if (com_ENet_initialize() != 0) {
    fprintf(stderr, "ENet initialization failed\n");
    exit(1);
}
```

#### `com_ENet_shutdown()`
Shuts down the ENet library. Call once at program exit.

**Example:**
```c
com_ENet_shutdown();
```

#### `com_ENet_get_version()`
Gets the ENet version string.

**Returns:** Version string (e.g., "ENet 1.3.18")

**Example:**
```c
const char* version = com_ENet_get_version();
printf("Using %s\n", version);
```

### Connection Functions

#### `com_ENet_open_host()`
Creates an ENet server (host) that accepts client connections.

**Parameters:**
- `buffer_size` - Size of send/receive buffers (recommended: 8192)
- `game_name` - Game identifier string (unused, can be NULL)
- `port` - Port number to listen on (e.g., 2934)
- `max_connections` - Maximum simultaneous clients (1-32)

**Returns:** Handle on success, NULL on failure

**Example:**
```c
com_API_handle host = com_ENet_open_host(8192, "FreeFalcon", 2934, 16);
```

#### `com_ENet_open_client()`
Connects to an ENet server.

**Parameters:**
- `buffer_size` - Size of send/receive buffers (recommended: 8192)
- `address` - Server IP address or hostname
- `port` - Server port number

**Returns:** Handle on success, NULL on failure

**Connection timeout:** 5 seconds

**Example:**
```c
com_API_handle client = com_ENet_open_client(8192, "server.example.com", 2934);
```

### Data Transfer Functions

#### `com_ENet_send()`
Sends data reliably to connected peer(s).

**Parameters:**
- `c` - Connection handle
- `buffer` - Data to send
- `len` - Number of bytes to send

**Returns:** Number of bytes sent, or negative error code

**Behavior:**
- **Host mode:** Broadcasts to all connected clients
- **Client mode:** Sends to server
- **Delivery:** Reliable (guaranteed delivery with retransmission)

**Example:**
```c
char data[256] = "Hello!";
int sent = com_ENet_send(handle, data, strlen(data));
if (sent < 0) {
    fprintf(stderr, "Send failed\n");
}
```

#### `com_ENet_recv()`
Receives data from connected peer(s).

**Parameters:**
- `c` - Connection handle
- `buffer` - Buffer to receive data into
- `len` - Maximum bytes to receive

**Returns:** Number of bytes received, 0 if no data, negative on error

**Behavior:**
- Non-blocking (returns immediately if no data)
- Processes connection events (connect/disconnect)
- Call frequently (in game loop)

**Example:**
```c
char buffer[8192];
int received = com_ENet_recv(handle, buffer, sizeof(buffer));
if (received > 0) {
    printf("Received %d bytes\n", received);
    process_data(buffer, received);
}
```

#### `com_ENet_close()`
Closes the connection and frees resources.

**Parameters:**
- `c` - Connection handle

**Behavior:**
- Gracefully disconnects peer(s)
- Waits up to 3 seconds for disconnect acknowledgment
- Frees all allocated memory

**Example:**
```c
com_ENet_close(handle);
```

### Query Functions

#### `com_ENet_query()`
Queries connection properties and statistics.

**Parameters:**
- `c` - Connection handle
- `query_type` - Type of query (see below)
- `result` - Pointer to store result

**Returns:** 0 on success, -1 on failure

**Query Types:**

| Query Type | Description | Result Type |
|------------|-------------|-------------|
| `COMAPI_PROTOCOL` | Get protocol ID | `int` (returns 12) |
| `COMAPI_STATE` | Get connection state | `int` (0=pending, 1=connected) |
| `COMAPI_SEND_MESSAGECOUNT` | Get sent message count | `unsigned long` |
| `COMAPI_RECV_MESSAGECOUNT` | Get received message count | `unsigned long` |
| `COMAPI_MAX_BUFFER_SIZE` | Get buffer size | `int` |
| `COMAPI_ACTUAL_BUFFER_SIZE` | Get buffer size | `int` |

**Example:**
```c
int state;
com_ENet_query(handle, COMAPI_STATE, &state);
if (state == COMAPI_STATE_CONNECTED) {
    printf("Connected!\n");
}

unsigned long sent_count;
com_ENet_query(handle, COMAPI_SEND_MESSAGECOUNT, &sent_count);
printf("Sent %lu messages\n", sent_count);
```

### COMAPI Interface Functions

These functions work with all protocols (TCP, UDP, ENet):

#### `ComAPISend()`
Send data through COMAPI interface (uses function pointers).

**Example:**
```c
char* buffer = ComAPISendBufferGet(handle);
strcpy(buffer, "Game data");
ComAPISend(handle, strlen(buffer), 0);
```

#### `ComAPIRecvBufferGet()`
Get receive buffer pointer.

**Example:**
```c
char* buffer = ComAPIRecvBufferGet(handle);
```

#### `ComAPISendBufferGet()`
Get send buffer pointer.

**Example:**
```c
char* buffer = ComAPISendBufferGet(handle);
```

#### `ComAPIClose()`
Close connection through COMAPI interface.

**Example:**
```c
ComAPIClose(handle);
```

#### `ComAPIQuery()`
Query through COMAPI interface.

**Example:**
```c
unsigned long protocol = ComAPIQuery(handle, COMAPI_PROTOCOL);
```

## Common Usage Patterns

### Game Loop Integration

```c
// Initialization (once at startup)
com_ENet_initialize();
com_API_handle net = is_server ?
    com_ENet_open_host(8192, "FreeFalcon", 2934, 16) :
    com_ENet_open_client(8192, server_address, 2934);

// Game loop
while (game_running) {
    // Send game state
    char* send_buf = ComAPISendBufferGet(net);
    int size = serialize_game_state(send_buf);
    ComAPISend(net, size, 0);

    // Receive updates
    char* recv_buf = ComAPIRecvBufferGet(net);
    int received = com_ENet_recv(net, recv_buf, 8192);
    if (received > 0) {
        deserialize_game_state(recv_buf, received);
    }

    // Update game
    update_game();
    render_game();
}

// Cleanup (once at shutdown)
ComAPIClose(net);
com_ENet_shutdown();
```

### Connection State Checking

```c
void check_connection(com_API_handle handle) {
    int state;
    com_ENet_query(handle, COMAPI_STATE, &state);

    switch (state) {
        case COMAPI_STATE_CONNECTION_PENDING:
            printf("Connecting...\n");
            break;
        case COMAPI_STATE_CONNECTED:
            printf("Connected\n");
            break;
        default:
            printf("Disconnected\n");
            break;
    }
}
```

### Message Statistics

```c
void print_statistics(com_API_handle handle) {
    unsigned long sent, received;

    com_ENet_query(handle, COMAPI_SEND_MESSAGECOUNT, &sent);
    com_ENet_query(handle, COMAPI_RECV_MESSAGECOUNT, &received);

    printf("Messages: Sent=%lu, Received=%lu\n", sent, received);
}
```

## Protocol Comparison

| Feature | ENet | TCP | UDP | DirectPlay |
|---------|------|-----|-----|------------|
| Reliable Delivery | ✅ Yes | ✅ Yes | ❌ No | ✅ Yes |
| Low Latency | ✅ Yes | ❌ No | ✅ Yes | ⚠️ Varies |
| Packet Ordering | ✅ Yes | ✅ Yes | ❌ No | ✅ Yes |
| Cross-platform | ✅ Yes | ✅ Yes | ✅ Yes | ❌ Windows only |
| Modern Support | ✅ Yes | ✅ Yes | ✅ Yes | ❌ Deprecated |
| NAT Friendly | ⚠️ Moderate | ✅ Yes | ⚠️ Moderate | ⚠️ Moderate |
| Bandwidth Efficient | ✅ Yes | ⚠️ Moderate | ✅ Yes | ⚠️ Moderate |

## Configuration

### Recommended Buffer Sizes

- **Small messages (<1KB):** 2048 bytes
- **Medium messages (1-4KB):** 4096 bytes
- **Large messages (4-8KB):** 8192 bytes (recommended)
- **Very large messages (8-16KB):** 16384 bytes

### Port Selection

Default FreeFalcon port: **2934**

For custom servers, choose a port:
- Range: 1024-49151 (registered ports)
- Avoid: Well-known ports (0-1023)
- Ensure: Port forwarding configured on router

### Connection Limits

- **Minimum clients:** 1
- **Maximum clients:** 32 (per server)
- **Recommended:** 8-16 for best performance

## Troubleshooting

### Connection Fails

**Problem:** `com_ENet_open_client()` returns NULL

**Solutions:**
1. Check server is running and listening
2. Verify correct IP address and port
3. Check firewall settings
4. Ensure ENet is initialized

### Port Already in Use

**Problem:** `com_ENet_open_host()` returns NULL

**Solutions:**
1. Close other applications using the port
2. Use a different port number
3. Check for zombie processes

### No Data Received

**Problem:** `com_ENet_recv()` always returns 0

**Solutions:**
1. Ensure data is being sent from other peer
2. Check connection state (must be CONNECTED)
3. Call `com_ENet_recv()` frequently (every frame)
4. Verify firewall isn't blocking UDP

### High Latency

**Problem:** Slow response times

**Solutions:**
1. Check network connection quality
2. Reduce message size
3. Send only changed data, not full state
4. Consider using smaller buffer sizes

## Advanced Topics

### Thread Safety

ENet backend uses mutexes for thread safety. Safe to call from multiple threads.

### Bandwidth Management

ENet automatically manages bandwidth. For manual control:
- Send smaller, more frequent updates
- Compress data before sending
- Only send changed values (delta compression)

### Multiple Channels

Current implementation uses single channel (channel 0). Future enhancement: multiple channels for different message priorities.

### NAT Traversal

ENet does not include built-in NAT traversal. For NAT:
- Use port forwarding on router
- Or implement hole punching separately
- Or use relay server

## Examples

### Complete Server Example

```c
#include "comms/capi.h"
#include <stdio.h>

int main(void) {
    // Initialize
    if (com_ENet_initialize() != 0) {
        fprintf(stderr, "Failed to initialize ENet\n");
        return 1;
    }

    printf("Starting FreeFalcon server...\n");

    // Create server
    com_API_handle server = com_ENet_open_host(8192, "FreeFalcon", 2934, 16);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        com_ENet_shutdown();
        return 1;
    }

    printf("Server listening on port 2934\n");
    printf("Waiting for clients...\n");

    // Server loop
    char buffer[8192];
    while (1) {
        // Receive from clients
        int received = com_ENet_recv(server, buffer, sizeof(buffer));
        if (received > 0) {
            printf("Received %d bytes from client\n", received);

            // Echo back to all clients
            char* send_buf = ComAPISendBufferGet(server);
            memcpy(send_buf, buffer, received);
            ComAPISend(server, received, 0);
        }

        Sleep(10); // 100 Hz update rate
    }

    // Cleanup
    ComAPIClose(server);
    com_ENet_shutdown();
    return 0;
}
```

### Complete Client Example

```c
#include "comms/capi.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    // Initialize
    if (com_ENet_initialize() != 0) {
        fprintf(stderr, "Failed to initialize ENet\n");
        return 1;
    }

    printf("Connecting to server %s...\n", argv[1]);

    // Connect to server
    com_API_handle client = com_ENet_open_client(8192, argv[1], 2934);
    if (!client) {
        fprintf(stderr, "Failed to connect to server\n");
        com_ENet_shutdown();
        return 1;
    }

    printf("Connected!\n");

    // Send a message
    char* send_buf = ComAPISendBufferGet(client);
    strcpy(send_buf, "Hello from FreeFalcon client!");
    ComAPISend(client, strlen(send_buf), 0);
    printf("Sent: %s\n", send_buf);

    // Wait for response
    char recv_buf[8192];
    int attempts = 50; // 5 seconds
    while (attempts-- > 0) {
        int received = com_ENet_recv(client, recv_buf, sizeof(recv_buf));
        if (received > 0) {
            recv_buf[received] = '\0';
            printf("Received: %s\n", recv_buf);
            break;
        }
        Sleep(100);
    }

    // Cleanup
    ComAPIClose(client);
    com_ENet_shutdown();
    return 0;
}
```

## Further Reading

- ENet Official Documentation: http://enet.bespin.org/Tutorial.html
- FreeFalcon COMAPI Guide: See `src/comms/capi.h`
- ENet Source Code: `src/extlibs/enet/`

## Support

For issues or questions:
1. Check this guide first
2. Review test examples in `src/test_enet.c`
3. Check ENet documentation
4. Report bugs to FreeFalcon project

---

**Document Version:** 1.0
**Last Updated:** 2026-01-16
**FreeFalcon Version:** Development (Phase 3)
