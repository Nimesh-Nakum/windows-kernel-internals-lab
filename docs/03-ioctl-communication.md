# 03 — IOCTL Communication: Ring 3 Talking to Ring 0

**Tools:** DebugView++, Process Hacker, CMD  
**Code:** `drivers/ioctl-driver/IoctlDriver.c`, `userland/talk-to-driver/TalkToDriver.cpp`  
**Time:** ~40 minutes  
**Snapshot before starting**

---

## Step 1 — Compile and Load the IOCTL Driver

Source: `drivers/ioctl-driver/IoctlDriver.c`

Compile the same way as Simulation 02.  
Copy output to `C:\IoctlDriver.sys`

Load it:

```cmd
sc create IoctlDriver type= kernel binPath= C:\IoctlDriver.sys
sc start IoctlDriver
```

**DebugView++ output:**

```
[IoctlDriver] Loaded — IOCTL device ready
```

Driver is in Ring 0. Device interface is open.

---

## Step 2 — Compile the User-Mode Client

Source: `userland/talk-to-driver/TalkToDriver.cpp`

Compile with any C++ compiler inside the VM:

```cmd
cl TalkToDriver.cpp /Fe:TalkToDriver.exe
```

Or use Visual Studio — create a new Console App, paste the source, build x64.

---

## Step 3 — Open Three Windows Side by Side

Set this up before running anything:

- **Window 1:** DebugView++ — watching kernel output
- **Window 2:** CMD — ready to run `TalkToDriver.exe`
- **Window 3:** Process Hacker — Services tab, driver visible

---

## Step 4 — Run the Client and Watch Both Sides

Run in Window 2:

```cmd
TalkToDriver.exe
```

**Ring 3 output (your console):**

```
[Ring 3] Opening handle to kernel driver...
[Ring 3] Handle opened — I can now talk to Ring 0

[Ring 3] Sending IOCTL_HELLO to Ring 0...
[Ring 3] Ring 0 replied: Hello Ring 3!

[Ring 3] Asking Ring 0 to add 42 + 58...
[Ring 3] Ring 0 computed: 42 + 58 = 100

[Ring 3] This is exactly how BYOVD works.
[Ring 3] Replace ADD with READ KERNEL MEMORY
[Ring 3] Replace HELLO with WRITE KERNEL MEMORY
[Ring 3] Same mechanism. Different payload.
```

**Ring 0 output (DebugView++ at the same time):**

```
[IoctlDriver] Loaded — IOCTL device ready
[IoctlDriver] Ring 3 said hello!
[IoctlDriver] Adding 42 + 58 = 100
```

---

## What You Just Saw

| What Happened | What It Maps To in BYOVD |
|---|---|
| `CreateFile("\\\\.\\IoctlDriver")` opened a handle | Attacker opens handle to vulnerable driver |
| `DeviceIoControl` sent IOCTL_HELLO | Attacker sends IOCTL request to driver |
| Driver processed it in Ring 0 and replied | Vulnerable driver executes attacker-supplied operation in kernel |
| `IOCTL_ADD` computed result in kernel | Replace "add numbers" with "read kernel memory address" |

The communication channel is identical.  
The only thing that changes between this simulation and a real BYOVD scenario is what the IOCTL operation *does* in Ring 0.

---

## Step 5 — Unload

```cmd
sc stop IoctlDriver
sc delete IoctlDriver
```

---

## Observed Output to Screenshot

- Both windows side by side: console (Ring 3) and DebugView++ (Ring 0) showing the same exchange from both ends
- Process Hacker showing the driver loaded during communication

---

**Next → [04-callbacks.md](./04-callbacks.md)**