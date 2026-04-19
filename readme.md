
# Windows Kernel Internals Lab

Hands-on simulations built alongside the blog post  
**"BYOVD Explained — How Attackers Use Signed Drivers to Kill EDRs"**

The blog explains the *why*. This repo shows the *what it looks like when you actually run it*.

Every simulation is isolated to a VM. Nothing here is an attack tool.  
You are here to **see** kernel internals behave — not to read about them.

---

## Read the Blog First

This repo will not make sense without the conceptual foundation.  
Blog → then lab. In that order.

> Blog: *BYOVD Explained — How Attackers Use Signed Drivers to Kill EDRs*  
> https://medium.com/@nimeshnakum3/byovd-explained-how-attackers-use-signed-drivers-to-kill-edrs-37a96bde4094

---

## What You Will Run

| # | File | What You See |
|---|---|---|
| 01 | `docs/01-ring3-vs-ring0.md` | Ring 3 vs Ring 0 boundary — live in Process Hacker and WinDbg |
| 02 | `docs/02-driver-loading.md` | A driver entering Ring 0 — address, output, and kernel space confirmed |
| 03 | `docs/03-ioctl-communication.md` | Ring 3 talking to Ring 0 via IOCTL — both sides live |
| 04 | `docs/04-callbacks.md` | Process callbacks firing on every launch — then going silent |
| 05 | `docs/05-minifilters.md` | File operations intercepted — then invisible |
| 06 | `docs/06-full-picture.md` | All simulations running together — the complete sequence |

---

## VM Setup (Do This Before Anything Else)

**Requirements**
- Windows 10 or Windows 11 VM — VMware or VirtualBox
- 4GB RAM minimum allocated to VM
- Take a snapshot before you start
- Do not use your host machine

**Tools — install inside VM only**

| Tool | Link |
|---|---|
| Process Hacker 2 | https://processhacker.sourceforge.io |
| WinDbg | Microsoft Store (free) |
| DebugView++ | https://github.com/CobaltFusion/DebugViewPP |
| x64dbg | https://x64dbg.com |
| Sysmon | https://learn.microsoft.com/en-us/sysinternals/downloads/sysmon |
| WDK + Visual Studio 2022 Community | https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk |

**One-time VM configuration**

```cmd
bcdedit /debug on
bcdedit /dbgsettings local
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
```
Reboot after running these. You will see a "Test Mode" watermark on the desktop — that is correct.

---

## Snapshot Rule

Snapshot before every simulation.  
If anything breaks — restore, do not troubleshoot on a running system.

---

## Repo Structure

```
windows-kernel-internals-lab/
├── README.md
├── docs/
│   ├── 01-ring3-vs-ring0.md
│   ├── 02-driver-loading.md
│   ├── 03-ioctl-communication.md
│   ├── 04-callbacks.md
│   ├── 05-minifilters.md
│   └── 06-full-picture.md
├── drivers/
│   ├── hello-driver/
│   │   ├── HelloDriver.c
│   │   └── README.md
│   └── ioctl-driver/
│       ├── IoctlDriver.c
│       └── README.md
├── userland/
│   └── talk-to-driver/
│       ├── TalkToDriver.cpp
│       └── README.md
├── screenshots/
│   ├── process-hacker.png
│   ├── windbg-output.png
│   └── debugview.png
└── notes/
└── observations.md
```
