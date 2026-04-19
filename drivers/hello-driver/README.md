# Hello Driver

The simplest possible kernel driver.  
Prints a debug message on load and unload. Nothing else.

**Purpose in this lab:**  
Proves that code can enter Ring 0, execute there, and be confirmed via kernel debug output and WinDbg memory addresses.

---

## What It Does

- On load: prints `[HelloDriver] Hello from Ring 0!` and logs its own `DriverObject` address
- On unload: prints `[HelloDriver] Unloaded from Ring 0`

---

## How to Compile

Requirements:
- Visual Studio 2022 Community
- Windows Driver Kit (WDK) — install WDK extension for Visual Studio after installing WDK

Steps:
1. Open Visual Studio → Create New Project → Empty WDM Driver
2. Add `HelloDriver.c` to the source files
3. Set configuration to x64 / Debug
4. Build

Output: `HelloDriver.sys` in the build output folder.

Copy to `C:\HelloDriver.sys` inside the VM.

---

## How to Load

Test signing must be enabled (one-time VM setup in main README):

```cmd
sc create HelloDriver type= kernel binPath= C:\HelloDriver.sys
sc start HelloDriver
```

Watch output in DebugView++ with **Capture Kernel** enabled.

## How to Unload

```cmd
sc stop HelloDriver
sc delete HelloDriver
```

---

## What to Observe

- `DebugView++` showing kernel debug output with `FFFF...` DriverObject address
- `WinDbg` → `lm m HelloDriver` showing driver loaded in kernel address range
- `Process Hacker` → Services tab showing Type = Kernel Driver, State = Running

---

## Used In

- `docs/02-driver-loading.md`
- `docs/06-full-picture.md`