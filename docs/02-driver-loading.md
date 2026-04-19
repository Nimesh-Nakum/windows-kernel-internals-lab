# 02 — Driver Loading: Watch Code Enter Ring 0

**Tools:** DebugView++, Process Hacker, WinDbg  
**Code:** `drivers/hello-driver/HelloDriver.c`  
**Time:** ~30 minutes  
**Snapshot before starting**

---

## Step 1 — Set Up DebugView++

Open DebugView++ as Administrator.  
Configure:
- Capture → **Capture Kernel** → ON  
- Capture → **Capture Global Win32** → ON  
- Filter → set to: `[HelloDriver]`

Leave it open. This is your live window into Ring 0 output.

---

## Step 2 — Compile the Driver

Source: `drivers/hello-driver/HelloDriver.c`

Open Visual Studio 2022 with WDK extension installed.  
Create a new **Empty WDM Driver** project.  
Add `HelloDriver.c` to the project.  
Build → x64 → Debug.

Output: `HelloDriver.sys`

Copy it to `C:\HelloDriver.sys` inside the VM.

> If you cannot compile: search GitHub for `"hello world kernel driver x64 compiled"`.  
> Several research repos provide pre-built test `.sys` files for this exact purpose.

---

## Step 3 — Load the Driver

Open CMD as Administrator:

```cmd
sc create HelloDriver type= kernel binPath= C:\HelloDriver.sys
sc start HelloDriver
```

**Watch DebugView++ immediately:**

```
[0001]  [HelloDriver] Hello from Ring 0!
[0002]  [HelloDriver] I am running in kernel mode
[0003]  [HelloDriver] DriverObject address: FFFF9A8B12345678
```

The `FFFF...` address — that is a Ring 0 kernel address.  
Your code is running in the kernel right now.

---

## Step 4 — Confirm in Process Hacker

While the driver is loaded, go to Process Hacker → **Services tab**.  
Find `HelloDriver`.  
Right-click → Properties:

```
Type:   Kernel Driver
State:  Running
Binary: C:\HelloDriver.sys
```

---

## Step 5 — Confirm in WinDbg

In WinDbg, run:

```
lm m HelloDriver
```

Output:

```
start             end                 module name
fffff800`12340000 fffff800`12345000   HelloDriver
```

`fffff800...` — your driver is loaded next to `ntoskrnl.exe` in kernel space.

---

## Step 6 — Unload and Watch

```cmd
sc stop HelloDriver
sc delete HelloDriver
```

**DebugView++ output:**

```
[0004]  [HelloDriver] Unloaded from Ring 0
```

---

## What You Just Saw

| Action | What Happened |
|---|---|
| `sc start` | Windows verified signature, loaded driver into Ring 0 |
| DebugView++ output | Code executing inside the kernel — `FFFF...` address confirms it |
| `lm m HelloDriver` in WinDbg | Driver sitting in kernel address space alongside OS components |
| `sc stop` | Driver unloaded, confirmed by kernel debug output |

---

## Observed Output to Screenshot

- DebugView++ showing `[HelloDriver]` kernel prints with `FFFF...` addresses
- WinDbg `lm m HelloDriver` output showing kernel address range
- Process Hacker Services tab showing driver state as Running

---

**Next → [03-ioctl-communication.md](./03-ioctl-communication.md)**