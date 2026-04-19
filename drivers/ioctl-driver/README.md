# IOCTL Driver

A kernel driver that exposes a device interface and handles two IOCTL requests from user mode.

**Purpose in this lab:**  
Shows the complete Ring 3 → Ring 0 communication channel.  
This is the same mechanism that vulnerable drivers in BYOVD scenarios expose — with a safe, observable payload instead of a destructive one.

---

## What It Does

Registers a device at `\\Device\\IoctlDriver` and handles two IOCTL codes:

| IOCTL | What Happens in Ring 0 |
|---|---|
| `IOCTL_HELLO` | Prints a message, replies with a string to Ring 3 |
| `IOCTL_ADD` | Receives two integers from Ring 3, adds them in kernel, returns result |

---

## How to Compile

Same requirements as `hello-driver`:
- Visual Studio 2022 Community + WDK extension

Steps:
1. Create Empty WDM Driver project in Visual Studio
2. Add `IoctlDriver.c`
3. Build x64 / Debug
4. Copy `IoctlDriver.sys` to `C:\IoctlDriver.sys` in VM

---

## How to Load

```cmd
sc create IoctlDriver type= kernel binPath= C:\IoctlDriver.sys
sc start IoctlDriver
```

**DebugView++ output:**
```
[IoctlDriver] Loaded — IOCTL device ready
```

## How to Unload

```cmd
sc stop IoctlDriver
sc delete IoctlDriver
```

---

## What to Observe

- DebugView++ shows kernel-side processing as each IOCTL arrives
- Console (Ring 3) shows the response returned from Ring 0
- Both sides of the conversation visible simultaneously — the complete channel

---

## Used In

- `docs/03-ioctl-communication.md`