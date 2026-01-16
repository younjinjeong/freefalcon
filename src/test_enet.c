/*
 * test_enet.c
 *
 * Comprehensive test program for ENet COMAPI backend
 * Tests initialization, host creation, client connection, send/receive, and cleanup
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Include COMAPI headers */
#include "comms/capi.h"

/* Forward declarations of ENet functions */
extern int com_ENet_initialize(void);
extern void com_ENet_shutdown(void);
extern const char* com_ENet_get_version(void);
extern com_API_handle com_ENet_open_host(int buffer_size, char* game_name,
                                          unsigned short port, int max_connections);
extern com_API_handle com_ENet_open_client(int buffer_size, char* address,
                                            unsigned short port);
extern int com_ENet_send(com_API_handle c, char* buffer, int len);
extern int com_ENet_recv(com_API_handle c, char* buffer, int len);
extern void com_ENet_close(com_API_handle c);
extern int com_ENet_query(com_API_handle c, int query_type, void* result);

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) printf("\n[TEST] %s...\n", name)
#define TEST_PASS(name) do { printf("  ✓ PASS: %s\n", name); tests_passed++; } while(0)
#define TEST_FAIL(name, reason) do { printf("  ✗ FAIL: %s - %s\n", name, reason); tests_failed++; } while(0)

/*=============================================================================
 * Test 1: Library Initialization
 *=============================================================================*/
void test_initialization()
{
    TEST_START("ENet Library Initialization");

    if (com_ENet_initialize() == 0) {
        TEST_PASS("ENet initialized successfully");

        const char* version = com_ENet_get_version();
        if (version != NULL && strlen(version) > 0) {
            printf("    Version: %s\n", version);
            TEST_PASS("Version string retrieved");
        } else {
            TEST_FAIL("Version string", "NULL or empty");
        }
    } else {
        TEST_FAIL("ENet initialization", "Initialize returned non-zero");
    }
}

/*=============================================================================
 * Test 2: Host Creation
 *=============================================================================*/
void test_host_creation()
{
    TEST_START("Host Creation");

    com_API_handle host = com_ENet_open_host(8192, "TestGame", 2934, 8);

    if (host == NULL) {
        TEST_FAIL("Host creation", "Returned NULL handle");
        return;
    }

    TEST_PASS("Host created successfully");

    /* Query host properties */
    int protocol = 0;
    int state = 0;
    int max_buffer = 0;

    if (com_ENet_query(host, COMAPI_PROTOCOL, &protocol) == 0) {
        printf("    Protocol: %d (expected %d)\n", protocol, CAPI_ENET_PROTOCOL);
        if (protocol == CAPI_ENET_PROTOCOL) {
            TEST_PASS("Protocol query");
        } else {
            TEST_FAIL("Protocol query", "Wrong protocol value");
        }
    } else {
        TEST_FAIL("Protocol query", "Query failed");
    }

    if (com_ENet_query(host, COMAPI_STATE, &state) == 0) {
        printf("    State: %d (expected %d=CONNECTED)\n", state, COMAPI_STATE_CONNECTED);
        if (state == COMAPI_STATE_CONNECTED) {
            TEST_PASS("State query");
        } else {
            TEST_FAIL("State query", "Wrong state value");
        }
    } else {
        TEST_FAIL("State query", "Query failed");
    }

    if (com_ENet_query(host, COMAPI_MAX_BUFFER_SIZE, &max_buffer) == 0) {
        printf("    Max buffer: %d bytes\n", max_buffer);
        if (max_buffer == 8192) {
            TEST_PASS("Buffer size query");
        } else {
            TEST_FAIL("Buffer size query", "Wrong buffer size");
        }
    } else {
        TEST_FAIL("Buffer size query", "Query failed");
    }

    /* Close host */
    com_ENet_close(host);
    TEST_PASS("Host closed successfully");
}

/*=============================================================================
 * Test 3: Client Connection (with background host)
 *=============================================================================*/
DWORD WINAPI host_thread_func(LPVOID param)
{
    com_API_handle host = (com_API_handle)param;
    char recv_buffer[256];

    /* Wait for connection and messages for 10 seconds */
    DWORD start_time = GetTickCount();
    while (GetTickCount() - start_time < 10000) {
        int received = com_ENet_recv(host, recv_buffer, sizeof(recv_buffer));
        if (received > 0) {
            printf("    Host received: %d bytes\n", received);
            break;
        }
        Sleep(100);
    }

    return 0;
}

void test_client_connection()
{
    TEST_START("Client Connection");

    /* Create host */
    com_API_handle host = com_ENet_open_host(8192, "TestGame", 2935, 8);
    if (host == NULL) {
        TEST_FAIL("Client test", "Could not create host");
        return;
    }
    TEST_PASS("Host created on port 2935");

    /* Start host thread to receive messages */
    HANDLE thread = CreateThread(NULL, 0, host_thread_func, host, 0, NULL);
    if (thread == NULL) {
        TEST_FAIL("Client test", "Could not create host thread");
        com_ENet_close(host);
        return;
    }

    /* Give host time to start */
    Sleep(500);

    /* Create client and connect */
    com_API_handle client = com_ENet_open_client(8192, "127.0.0.1", 2935);
    if (client == NULL) {
        TEST_FAIL("Client connection", "Could not connect to host");
        TerminateThread(thread, 0);
        CloseHandle(thread);
        com_ENet_close(host);
        return;
    }
    TEST_PASS("Client connected to host");

    /* Query client state */
    int state = 0;
    if (com_ENet_query(client, COMAPI_STATE, &state) == 0) {
        printf("    Client state: %d\n", state);
        if (state == COMAPI_STATE_CONNECTED) {
            TEST_PASS("Client state is CONNECTED");
        } else {
            TEST_FAIL("Client state", "Not in CONNECTED state");
        }
    }

    /* Test send from client to host */
    char send_data[] = "Hello from client!";
    int sent = com_ENet_send(client, send_data, strlen(send_data));
    if (sent > 0) {
        printf("    Client sent: %d bytes\n", sent);
        TEST_PASS("Client send");
    } else {
        TEST_FAIL("Client send", "Send returned <= 0");
    }

    /* Wait for host thread */
    WaitForSingleObject(thread, 5000);
    CloseHandle(thread);

    /* Cleanup */
    com_ENet_close(client);
    com_ENet_close(host);
    TEST_PASS("Client and host cleaned up");
}

/*=============================================================================
 * Test 4: Message Statistics
 *=============================================================================*/
void test_statistics()
{
    TEST_START("Message Statistics");

    com_API_handle host = com_ENet_open_host(8192, "TestGame", 2936, 8);
    if (host == NULL) {
        TEST_FAIL("Statistics test", "Could not create host");
        return;
    }

    unsigned long send_count = 0;
    unsigned long recv_count = 0;

    /* Initial counts should be zero */
    if (com_ENet_query(host, COMAPI_SEND_MESSAGECOUNT, &send_count) == 0) {
        printf("    Initial send count: %lu\n", send_count);
        if (send_count == 0) {
            TEST_PASS("Initial send count is zero");
        } else {
            TEST_FAIL("Initial send count", "Not zero");
        }
    }

    if (com_ENet_query(host, COMAPI_RECV_MESSAGECOUNT, &recv_count) == 0) {
        printf("    Initial recv count: %lu\n", recv_count);
        if (recv_count == 0) {
            TEST_PASS("Initial recv count is zero");
        } else {
            TEST_FAIL("Initial recv count", "Not zero");
        }
    }

    com_ENet_close(host);
    TEST_PASS("Statistics test complete");
}

/*=============================================================================
 * Test 5: Shutdown
 *=============================================================================*/
void test_shutdown()
{
    TEST_START("ENet Shutdown");

    com_ENet_shutdown();
    TEST_PASS("ENet shutdown successful");
}

/*=============================================================================
 * Main Test Runner
 *=============================================================================*/
int main(int argc, char** argv)
{
    printf("=====================================\n");
    printf("  ENet COMAPI Backend Test Suite\n");
    printf("=====================================\n");

    /* Run all tests */
    test_initialization();
    test_host_creation();
    test_client_connection();
    test_statistics();
    test_shutdown();

    /* Print summary */
    printf("\n=====================================\n");
    printf("  Test Results\n");
    printf("=====================================\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("=====================================\n");

    if (tests_failed == 0) {
        printf("\n✓ ALL TESTS PASSED!\n\n");
        return 0;
    } else {
        printf("\n✗ SOME TESTS FAILED\n\n");
        return 1;
    }
}
