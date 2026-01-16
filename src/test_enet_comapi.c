/*
 * test_enet_comapi.c
 *
 * Test ENet backend through COMAPI interface functions
 * This tests that the function pointers are correctly initialized
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
extern com_API_handle com_ENet_open_host(int buffer_size, char* game_name,
                                          unsigned short port, int max_connections);
extern com_API_handle com_ENet_open_client(int buffer_size, char* address,
                                            unsigned short port);

/* COMAPI interface functions (from capi.c) */
extern void ComAPIClose(com_API_handle c);
extern int ComAPISend(com_API_handle c, int msgsize, int type);
extern int ComAPIGet(com_API_handle c, char **buffer, int *bytes);
extern char* ComAPIRecvBufferGet(com_API_handle c);
extern char* ComAPISendBufferGet(com_API_handle c);
extern unsigned long ComAPIQuery(com_API_handle c, int querytype);

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) printf("\n[TEST] %s...\n", name)
#define TEST_PASS(name) do { printf("  ✓ PASS: %s\n", name); tests_passed++; } while(0)
#define TEST_FAIL(name, reason) do { printf("  ✗ FAIL: %s - %s\n", name, reason); tests_failed++; } while(0)

/*=============================================================================
 * Test 1: COMAPI Send/Recv Buffer Functions
 *=============================================================================*/
void test_comapi_buffers()
{
    TEST_START("COMAPI Buffer Functions");

    com_ENet_initialize();

    com_API_handle host = com_ENet_open_host(8192, "TestGame", 2940, 8);
    if (host == NULL) {
        TEST_FAIL("Buffer test", "Could not create host");
        return;
    }

    /* Get send buffer through COMAPI */
    char* send_buf = ComAPISendBufferGet(host);
    if (send_buf != NULL) {
        TEST_PASS("ComAPISendBufferGet returned valid pointer");

        /* Write test data */
        strcpy(send_buf, "Test message via COMAPI");
        TEST_PASS("Wrote to send buffer");
    } else {
        TEST_FAIL("ComAPISendBufferGet", "Returned NULL");
    }

    /* Get receive buffer through COMAPI */
    char* recv_buf = ComAPIRecvBufferGet(host);
    if (recv_buf != NULL) {
        TEST_PASS("ComAPIRecvBufferGet returned valid pointer");
    } else {
        TEST_FAIL("ComAPIRecvBufferGet", "Returned NULL");
    }

    ComAPIClose(host);
    TEST_PASS("Host closed via ComAPIClose");

    com_ENet_shutdown();
}

/*=============================================================================
 * Test 2: COMAPI Query Functions
 *=============================================================================*/
void test_comapi_query()
{
    TEST_START("COMAPI Query Functions");

    com_ENet_initialize();

    com_API_handle host = com_ENet_open_host(8192, "TestGame", 2941, 8);
    if (host == NULL) {
        TEST_FAIL("Query test", "Could not create host");
        return;
    }

    /* Query protocol through COMAPI */
    unsigned long protocol = ComAPIQuery(host, COMAPI_PROTOCOL);
    printf("    Protocol via ComAPIQuery: %lu (expected %d)\n", protocol, CAPI_ENET_PROTOCOL);
    if (protocol == CAPI_ENET_PROTOCOL) {
        TEST_PASS("ComAPIQuery(COMAPI_PROTOCOL)");
    } else {
        TEST_FAIL("ComAPIQuery(COMAPI_PROTOCOL)", "Wrong protocol");
    }

    /* Query state through COMAPI */
    unsigned long state = ComAPIQuery(host, COMAPI_STATE);
    printf("    State via ComAPIQuery: %lu (expected %d)\n", state, COMAPI_STATE_CONNECTED);
    if (state == COMAPI_STATE_CONNECTED) {
        TEST_PASS("ComAPIQuery(COMAPI_STATE)");
    } else {
        TEST_FAIL("ComAPIQuery(COMAPI_STATE)", "Wrong state");
    }

    /* Query buffer size through COMAPI */
    unsigned long buffer_size = ComAPIQuery(host, COMAPI_MAX_BUFFER_SIZE);
    printf("    Buffer size via ComAPIQuery: %lu bytes\n", buffer_size);
    if (buffer_size == 8192) {
        TEST_PASS("ComAPIQuery(COMAPI_MAX_BUFFER_SIZE)");
    } else {
        TEST_FAIL("ComAPIQuery(COMAPI_MAX_BUFFER_SIZE)", "Wrong size");
    }

    ComAPIClose(host);
    com_ENet_shutdown();
}

/*=============================================================================
 * Test 3: COMAPI Send via Function Pointers
 *=============================================================================*/
/* Forward declare internal recv function */
extern int com_ENet_recv(com_API_handle c, char* buffer, int len);

DWORD WINAPI comapi_host_thread(LPVOID param)
{
    com_API_handle host = (com_API_handle)param;
    char buffer[256];

    /* Poll for messages */
    DWORD start_time = GetTickCount();
    while (GetTickCount() - start_time < 10000) {
        int received = com_ENet_recv(host, buffer, sizeof(buffer));
        if (received > 0) {
            printf("    Host received: %d bytes\n", received);
            printf("    Message: '%s'\n", buffer);
            break;
        }
        Sleep(100);
    }

    return 0;
}

void test_comapi_send()
{
    TEST_START("COMAPI Send/Recv via Function Pointers");

    com_ENet_initialize();

    /* Create host */
    com_API_handle host = com_ENet_open_host(8192, "TestGame", 2942, 8);
    if (host == NULL) {
        TEST_FAIL("Send test", "Could not create host");
        return;
    }
    TEST_PASS("Host created");

    /* Start host thread */
    HANDLE thread = CreateThread(NULL, 0, comapi_host_thread, host, 0, NULL);
    Sleep(500);

    /* Create client */
    com_API_handle client = com_ENet_open_client(8192, "127.0.0.1", 2942);
    if (client == NULL) {
        TEST_FAIL("Send test", "Could not create client");
        TerminateThread(thread, 0);
        CloseHandle(thread);
        ComAPIClose(host);
        return;
    }
    TEST_PASS("Client created");

    /* Send message via ComAPISend */
    char* send_buffer = ComAPISendBufferGet(client);
    strcpy(send_buffer, "Message via ComAPISend!");
    int sent = ComAPISend(client, strlen(send_buffer), 0);

    if (sent > 0) {
        printf("    Client sent via ComAPISend: %d bytes\n", sent);
        TEST_PASS("ComAPISend succeeded");
    } else {
        TEST_FAIL("ComAPISend", "Failed to send");
    }

    /* Wait for host thread */
    WaitForSingleObject(thread, 5000);
    CloseHandle(thread);

    /* Cleanup via COMAPI */
    ComAPIClose(client);
    ComAPIClose(host);
    TEST_PASS("Cleanup via ComAPIClose");

    com_ENet_shutdown();
}

/*=============================================================================
 * Main Test Runner
 *=============================================================================*/
int main(int argc, char** argv)
{
    printf("=============================================\n");
    printf("  ENet COMAPI Function Pointer Test Suite\n");
    printf("=============================================\n");

    /* Run all tests */
    test_comapi_buffers();
    test_comapi_query();
    test_comapi_send();

    /* Print summary */
    printf("\n=============================================\n");
    printf("  Test Results\n");
    printf("=============================================\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_passed + tests_failed);
    printf("=============================================\n");

    if (tests_failed == 0) {
        printf("\n✓ ALL COMAPI TESTS PASSED!\n\n");
        return 0;
    } else {
        printf("\n✗ SOME TESTS FAILED\n\n");
        return 1;
    }
}
