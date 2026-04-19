# 04 — Kernel Callbacks: Watch EDR Eyes Open and Close

**Tools:** DebugView++, Process Hacker  
**Code:** `drivers/hello-driver/HelloDriver.c` (callback variant — see note below)  
**Time:** ~30 minutes  
**Snapshot before starting**

---

## Note on Source

The callback driver source registers `PsSetCreateProcessNotifyRoutineEx`.  
This requires the driver to be compiled with `/integritycheck` linker flag in WDK.  
If compilation fails on this flag, search GitHub for  
`"PsSetCreateProcessNotifyRoutineEx example driver"` — several research repos provide working compiled examples.

---

## Step 1 — Load the Callback Driver

```cmd
sc create EDR_SIM type= kernel binPath= C:\EDR_SIM.sys
sc start EDR_SIM
```

**DebugView++ output:**

```
[EDR_SIM] Process callback registered — EDR watching
```

The callback is now registered in the kernel.  
Every process creation on this machine will fire through it.

---

## Step 2 — Watch the Callback Fire on Every Launch

With DebugView++ open, launch applications one by one:

- Open Notepad
- Open CMD
- Open Chrome or any browser

**DebugView++ output:**

```
[EDR_SIM] New process created: \Device\HarddiskVolume3\Windows\System32\notepad.exe (PID: 4521)
[EDR_SIM] New process created: \Device\HarddiskVolume3\Windows\System32\cmd.exe (PID: 3301)
[EDR_SIM] New process created: \Device\HarddiskVolume3\Program Files\Chrome\chrome.exe (PID: 5832)
```

Every launch. Every one. The callback fires before the process has a chance to execute.

---

## Step 3 — Test the Block Logic

Create a file called `mimikatz.exe` anywhere on the VM — it can be empty or a copy of Notepad renamed.  
Try to open it.

**DebugView++ output:**

```
[EDR_SIM] New process created: ...\mimikatz.exe (PID: 7123)
[EDR_SIM] *** THREAT DETECTED: mimikatz ***
[EDR_SIM] *** BLOCKING PROCESS CREATION ***
```

The process never opens. Blocked at the callback level — in Ring 0 — before it executed a single instruction.

---

## Step 4 — Kill the Callback and Watch It Go Blind

```cmd
sc stop EDR_SIM
```

**DebugView++ output:**

```
[EDR_SIM] Callback unregistered — EDR blind now
```

Now try to open `mimikatz.exe` again.

**DebugView++ output:**

```
(nothing)
```

The process opens. No alert. No log. No block.  
The callback is gone. The detection does not exist.

---

## Step 5 — Confirm EDR Process Is Still "Running"

Go to Process Hacker.  
The `EDR_SIM` service entry is still there — state shows Stopped because we unloaded it manually.  

In a real BYOVD scenario: the EDR process keeps running in Ring 3.  
Only its kernel hooks are removed. The dashboard stays green.  
This simulation shows *why* — the detection lived in the callback, not in the process.

---

## What You Just Saw

| Action | Result |
|---|---|
| Callback registered | Every process launch visible and blockable |
| Callback unregistered | Zero visibility — no logs, no blocks, no alerts |
| EDR process state | Irrelevant — detection lives in the kernel hook, not the process |

---

## Observed Output to Screenshot

- DebugView++ flooded with process names while callback is active
- DebugView++ completely silent after `sc stop` — same process launches, zero output
- `mimikatz.exe` blocked while active, opens freely after callback removed

---

**Next → [05-minifilters.md](./05-minifilters.md)**