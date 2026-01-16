# ENet Integration Guide for FreeFalcon

## Overview

This guide explains how to integrate the ENet protocol into FreeFalcon's existing networking architecture. Based on codebase exploration, FreeFalcon uses a **COMAPI abstraction layer** that makes adding new protocols straightforward.

**Goal:** Enable FreeFalcon to use ENet as an alternative to UDP/TCP/DirectPlay protocols.

## Current Architecture

### How FreeFalcon Networking Works

```
UI Layer (phonebk.cpp)
    ↓ (User selects server)
ComDataClass (connection params)
    ↓
InitCommsStuff() (f4comms.cpp)
    ↓ (Creates protocol handles)
ComAPI Group Handles
    ↓
VU2 System (vu_thread.cpp)
    ↓ (Send/Receive game messages)
ComAPISend/ComAPIGet
    ↓
Protocol Implementation (TCP/UDP/RUDP/ENet)
```

### Key Files

| File | Purpose | Changes Needed |
|------|---------|----------------|
| `src/falclib/f4comms.cpp` | Connection initialization | ✅ Add ENet path |
| `src/comms/comenet.c` | ENet implementation | ✅ Already exists |
| `src/ui/src/comms/phonebk.cpp` | Server selection UI | ⚠️ Add protocol selector |
| `src/falclib/include/f4comms.h` | Protocol constants | ⚠️ Add ENet constant |
| `src/comms/capi.c` | Protocol enumeration | ⚠️ Optional enhancement |

## Integration Steps

### Step 1: Add ENet Protocol Constant

**File:** `src/falclib/include/f4comms.h`

**Current code** (lines ~60-65):
```cpp
// FalconConnectionProtocol flags
#define FCP_NOTHING         0x00
#define FCP_TCP_AVAILABLE   0x01
#define FCP_UDP_AVAILABLE   0x02
#define FCP_RUDP_AVAILABLE  0x04
```

**Add:**
```cpp
#define FCP_ENET_AVAILABLE  0x08
```

---

### Step 2: Modify Connection Initialization

**File:** `src/falclib/f4comms.cpp`

**Current code** (lines 136-222 in `InitCommsStuff()`):
```cpp
void InitCommsStuff(ComDataClass *comData)
{
    // ... existing setup ...

    // Currently hardcoded to UDP/RUDP
    FalconGlobalUDPHandle = ComAPICreateGroup(...);
    FalconGlobalTCPHandle = ComAPICreateGroup(...);

    FalconConnectionProtocol = FCP_UDP_AVAILABLE bitor FCP_RUDP_AVAILABLE;
}
```

**Modified code:**
```cpp
void InitCommsStuff(ComDataClass *comData)
{
    // ... existing setup ...

    // Get selected protocol from comData
    int selectedProtocol = comData->protocolType; // New field

    if (selectedProtocol == CAPI_ENET_PROTOCOL) {
        // === ENET PATH (NEW) ===

        // Initialize ENet
        if (com_ENet_initialize() != 0) {
            MonoPrint("Failed to initialize ENet\n");
            return;
        }

        if (comData->isHost) {
            // Host mode - create ENet server
            com_API_handle enetHost = com_ENet_open_host(
                F4CommsMaxUDPMessageSize,  // Buffer size
                "FreeFalcon",               // Game name
                comData->portNumber,        // Port (e.g., 2934)
                comData->maxPlayers         // Max clients (e.g., 16)
            );

            if (!enetHost) {
                MonoPrint("Failed to create ENet host\n");
                return;
            }

            // Create group handle for ENet
            FalconGlobalUDPHandle = ComAPICreateGroup(
                "ENet GROUP",
                F4CommsMaxUDPMessageSize,
                0
            );

            // Add ENet handle to group
            ComAPIAddToGroup(FalconGlobalUDPHandle, enetHost);

            MonoPrint("ENet server started on port %d\n", comData->portNumber);

        } else {
            // Client mode - connect to ENet server
            com_API_handle enetClient = com_ENet_open_client(
                F4CommsMaxUDPMessageSize,  // Buffer size
                comData->ipAddress,         // Server IP
                comData->portNumber         // Server port
            );

            if (!enetClient) {
                MonoPrint("Failed to connect to ENet server\n");
                return;
            }

            // Create group handle for ENet
            FalconGlobalUDPHandle = ComAPICreateGroup(
                "ENet GROUP",
                F4CommsMaxUDPMessageSize,
                0
            );

            // Add ENet handle to group
            ComAPIAddToGroup(FalconGlobalUDPHandle, enetClient);

            MonoPrint("Connected to ENet server %s:%d\n",
                     comData->ipAddress, comData->portNumber);
        }

        // Set protocol flag
        FalconConnectionProtocol = FCP_ENET_AVAILABLE;

        // TCP handle not needed for ENet (uses single reliable channel)
        FalconGlobalTCPHandle = NULL;

    } else {
        // === EXISTING UDP/RUDP/TCP PATH ===

        // ... existing UDP/RUDP code ...
        FalconGlobalUDPHandle = ComAPICreateGroup(...);
        FalconGlobalTCPHandle = ComAPICreateGroup(...);
        FalconConnectionProtocol = FCP_UDP_AVAILABLE bitor FCP_RUDP_AVAILABLE;
    }

    // ... rest of function unchanged ...
}
```

**Required includes at top of file:**
```cpp
#include "comms/comenet.h"  // Add this line
```

---

### Step 3: Add Protocol Selection to ComDataClass

**File:** `src/falclib/include/f4comms.h`

**Current ComDataClass structure:**
```cpp
class ComDataClass {
public:
    char ipAddress[32];
    int portNumber;
    int maxPlayers;
    int isHost;
    // ... other fields ...
};
```

**Add:**
```cpp
    int protocolType;  // NEW: CAPI_TCP_PROTOCOL, CAPI_UDP_PROTOCOL, CAPI_ENET_PROTOCOL, etc.
```

**Constructor initialization:**
```cpp
ComDataClass() {
    // ... existing initialization ...
    protocolType = CAPI_UDP_PROTOCOL;  // Default to UDP
}
```

---

### Step 4: Add Protocol Selection UI (Optional)

**File:** `src/ui/src/comms/phonebk.cpp`

This step adds a UI control to let users choose the protocol.

**Location:** `Phone_Connect_CB()` function (around line 293)

**Current UI:**
- Text field for IP address
- Text field for port
- Connect button

**Enhanced UI with protocol selection:**

```cpp
void BuildPhonebookUI(long ID, short Type, C_Base *control)
{
    // ... existing UI controls ...

    // === ADD PROTOCOL SELECTOR ===

    // Create radio buttons or dropdown for protocol selection
    C_ListBox *protocolList = new C_ListBox;
    protocolList->Setup(ID_PROTOCOL_LIST, x, y, width, height);
    protocolList->SetFlags(C_BIT_ENABLED | C_BIT_USELINE);

    // Add protocol options
    protocolList->AddItem(CAPI_UDP_PROTOCOL, "UDP (Legacy)");
    protocolList->AddItem(CAPI_TCP_PROTOCOL, "TCP");
    protocolList->AddItem(CAPI_ENET_PROTOCOL, "ENet (Recommended)");

    // Set default selection
    protocolList->SetValue(CAPI_ENET_PROTOCOL);

    window->AddControl(protocolList);

    // ... rest of UI setup ...
}

void Phone_Connect_CB(long ID, short hittype, C_Base *control)
{
    // ... existing code to get IP/port ...

    // === GET SELECTED PROTOCOL ===
    C_ListBox *protocolList = (C_ListBox*)control->GetParent()->FindControl(ID_PROTOCOL_LIST);
    if (protocolList) {
        localData.protocolType = protocolList->GetValue();
    } else {
        localData.protocolType = CAPI_ENET_PROTOCOL;  // Default
    }

    // Pass to comms manager
    gCommsMgr->StartComms(&localData);
}
```

**Note:** The exact UI implementation depends on FreeFalcon's UI framework. The above is pseudocode showing the concept.

---

### Step 5: Add Shutdown Cleanup

**File:** `src/falclib/f4comms.cpp`

**Function:** `CleanupCommsStuff()` or equivalent shutdown function

**Add:**
```cpp
void CleanupCommsStuff()
{
    // ... existing cleanup ...

    // Shutdown ENet if it was used
    if (FalconConnectionProtocol & FCP_ENET_AVAILABLE) {
        com_ENet_shutdown();
        MonoPrint("ENet shutdown\n");
    }
}
```

---

### Step 6: (Optional) Add Protocol Enumeration

**File:** `src/comms/capi.c`

**Function:** `com_API_enum_protocols()` (currently commented out with `#if 0`)

If this function is actually used, uncomment and update it:

```cpp
int com_API_enum_protocols(int *protocols, int max_protocols)
{
    WSADATA wsaData;
    int count = 0;

    if (InitWS2(&wsaData) == 0) {
        return 0;
    }

    // TCP
    if (count < max_protocols) {
        protocols[count++] = CAPI_TCP_PROTOCOL;
    }

    // UDP
    if (count < max_protocols) {
        protocols[count++] = CAPI_UDP_PROTOCOL;
    }

    // RUDP
    if (count < max_protocols) {
        protocols[count++] = CAPI_RUDP_PROTOCOL;
    }

    // ENet (NEW)
    if (count < max_protocols) {
        protocols[count++] = CAPI_ENET_PROTOCOL;
    }

    return count;
}
```

---

## Testing the Integration

### Unit Test

Use the existing test program to verify ENet works:

```bash
cd build/x86/debug_win32/test_enet
./test_enet.exe
```

Expected output: All 16 tests pass.

### Integration Test

1. **Build FreeFalcon** with the changes above
2. **Host Test:**
   - Run FreeFalcon
   - Go to multiplayer menu
   - Select "Host Game"
   - Choose "ENet" protocol
   - Start hosting
3. **Client Test:**
   - Run another FreeFalcon instance
   - Go to multiplayer menu
   - Select "Join Game"
   - Enter host IP address
   - Choose "ENet" protocol
   - Connect
4. **Verify:**
   - Client successfully connects
   - Both can see each other in game
   - Messages are sent/received
   - Game plays normally

### Debug Output

Add debug prints to track protocol selection:

```cpp
MonoPrint("Protocol selected: %d\n", comData->protocolType);
MonoPrint("ENet host created: %p\n", enetHost);
MonoPrint("Connection protocol flags: 0x%02X\n", FalconConnectionProtocol);
```

---

## Minimal Integration (Quick Start)

If you want to test ENet with **minimal changes**, use this approach:

### Quick Integration Code

**File:** `src/falclib/f4comms.cpp`

**Replace the protocol creation section:**

```cpp
void InitCommsStuff(ComDataClass *comData)
{
    // ... existing setup until line ~169 ...

    // === QUICK ENET INTEGRATION ===

    // Initialize ENet
    com_ENet_initialize();

    // Create ENet host or client
    com_API_handle enet;
    if (comData->isHost) {
        enet = com_ENet_open_host(8192, "FreeFalcon", 2934, 16);
        MonoPrint("ENet HOST mode\n");
    } else {
        enet = com_ENet_open_client(8192, comData->ipAddress, 2934);
        MonoPrint("ENet CLIENT mode\n");
    }

    // Wrap in group handle
    FalconGlobalUDPHandle = ComAPICreateGroup("ENet GROUP", 8192, 0);
    ComAPIAddToGroup(FalconGlobalUDPHandle, enet);

    // No TCP handle needed
    FalconGlobalTCPHandle = NULL;

    // Set protocol
    FalconConnectionProtocol = FCP_UDP_AVAILABLE;  // Reuse UDP flag

    // ... rest of function unchanged ...
}
```

This forces ENet for all multiplayer sessions without UI changes.

---

## Configuration Options

### Default Port

**Current:** UDP uses `CAPI_UDP_PORT` (defined in `capi.h`)

**For ENet:** Use the same port or define a new one:

```cpp
#define CAPI_ENET_PORT 2934  // Standard FreeFalcon port
```

### Buffer Sizes

**Current values** (from `f4comms.cpp`):
```cpp
F4CommsMaxUDPMessageSize = 1024;  // Unreliable messages
F4CommsMaxTCPMessageSize = 4096;  // Reliable messages
```

**For ENet:** Use TCP size (reliable):
```cpp
int enetBufferSize = F4CommsMaxTCPMessageSize;  // 4096
```

### Max Players

**Current:** Varies by game mode (typically 4-16)

**For ENet:** Support up to 32 players:
```cpp
int maxPlayers = min(comData->maxPlayers, 32);
```

---

## Troubleshooting

### Issue: ENet functions not found (linker errors)

**Solution:** Ensure `comms.lib` is linked:
```
#pragma comment(lib, "comms.lib")
```

### Issue: Connection fails

**Check:**
1. ENet initialized? (`com_ENet_initialize()` called)
2. Correct IP and port?
3. Firewall blocking UDP?
4. Host started before client connects?

### Issue: No data received

**Check:**
1. `com_ENet_recv()` called regularly (every frame)?
2. Connection state is CONNECTED?
3. Data actually being sent from other peer?

---

## Benefits of This Integration

### For Players
- ✅ Better performance (lower latency than TCP)
- ✅ More reliable than UDP
- ✅ Cross-platform multiplayer (Windows, Linux, macOS)
- ✅ No DirectPlay dependency

### For Developers
- ✅ Drop-in replacement (uses existing COMAPI)
- ✅ No game logic changes required
- ✅ Modern, maintained library
- ✅ Easy to debug and maintain

---

## Next Steps

1. ✅ Implement Step 1-2 (minimal integration)
2. ⏳ Test with 2 players
3. ⏳ Add UI protocol selector (Step 4)
4. ⏳ Test with 4-16 players
5. ⏳ Performance testing and optimization
6. ⏳ Update user documentation

---

## Reference

- **ENet Protocol Guide:** See `ENET_PROTOCOL_GUIDE.md`
- **ENet Backend Code:** `src/comms/comenet.c`
- **Test Program:** `src/test_enet.c`
- **Migration Guide:** See `ENET_MIGRATION_GUIDE.md`

---

**Document Version:** 1.0
**Last Updated:** 2026-01-16
**Status:** Ready for implementation
