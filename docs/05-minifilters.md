# 05 — Minifilters: Watch File Operations Get Intercepted — Then Disappear

**Tools:** DebugView++, fltMC (built into Windows), Process Hacker  
**Time:** ~25 minutes  
**Snapshot before starting**

---

## Step 1 — See Your Current Minifilter Stack

Open CMD as Administrator:

```cmd
fltMC
```

**Output:**

```
Filter Name                     Num Instances    Altitude    Frame
------------------------------  -------------  ------------  -----
WdFilter                              3          328010         0
storqosflt                            1          244000         0
wcifs                                 1          189900         0
CldFlt                                1          180451         0
FileInfo                              4            45000        0
```

**What you are reading:**

| Column | Meaning |
|---|---|
| Filter Name | The minifilter driver name |
| Altitude | Execution order — higher number runs first |
| `WdFilter` at 328010 | Windows Defender sees every file operation before anything else |

Every file open, read, write, and create passes through `WdFilter` at altitude 328010  
before it reaches the disk. Nothing bypasses it unless the filter is gone.

---

## Step 2 — Load Your Simulation Minifilter

Compile `drivers/ioctl-driver/IoctlDriver.c` — or use the minifilter variant noted in  
`drivers/hello-driver/README.md` — and load it:

```cmd
sc create FILTER_SIM type= kernel binPath= C:\FILTER_SIM.sys
sc start FILTER_SIM
```

**DebugView++ output:**

```
[FILTER_SIM] Minifilter registered — watching all files
```

---

## Step 3 — Watch File Operations Flood In

With DebugView++ running, open any file — a text file, an image, anything.  
Browse a folder. Save a document.

**DebugView++ output:**

```
[FILTER_SIM] File access: \Device\HarddiskVolume3\Users\User\Desktop\notes.txt
[FILTER_SIM] File access: \Device\HarddiskVolume3\Windows\System32\kernel32.dll
[FILTER_SIM] File access: \Device\HarddiskVolume3\Users\User\Documents\report.docx
```

Every single file operation — intercepted before it completes.  
This is the minifilter doing exactly what `WdFilter` does for Defender.

---

## Step 4 — Confirm Stack Position in fltMC

While FILTER_SIM is loaded, run:

```cmd
fltMC
```

You will see your filter in the list with its registered altitude.  
Every file operation flows down through each altitude in order.

---

## Step 5 — Remove the Filter and Watch It Go Silent

```cmd
sc stop FILTER_SIM
```

**DebugView++ output:**

```
[FILTER_SIM] Minifilter unregistered — file ops invisible
```

Now open and save files again.

**DebugView++ output:**

```
(nothing)
```

Same file operations. Completely invisible.  
If this were `WdFilter` — Defender's filter — file-based detection is gone.

---

## Step 6 — Confirm Stack in fltMC Again

```cmd
fltMC
```

Your filter is gone from the list.  
`WdFilter` is still there — because we only removed our simulation.

---

## What You Just Saw

| State | File Visibility |
|---|---|
| Filter loaded | Every file open/read/write intercepted and logged |
| Filter unloaded | Zero interception — operations complete without any callback |
| Real world equivalent | Removing `WdFilter` from this stack = Defender cannot scan on access |

---

## Observed Output to Screenshot

- `fltMC` output showing full filter stack including `WdFilter` at altitude 328010
- DebugView++ flooded with file paths while FILTER_SIM active
- DebugView++ silent after unload with same file activity happening

---

**Next → [06-full-picture.md](./06-full-picture.md)**