# 06 — Full Picture: All Simulations Running Together

**Tools:** DebugView++, Process Hacker, CMD, WinDbg, Event Viewer  
**Time:** ~45 minutes  
**Snapshot before starting — this is the most important one**

Run all four windows simultaneously before starting any scene.

---

## Window Layout

| Window | Tool | Watching |
|---|---|---|
| 1 | DebugView++ | All kernel output — every simulation feeds here |
| 2 | Process Hacker | Services tab — driver load/unload state |
| 3 | CMD (Admin) | Attacker console — all commands run here |
| 4 | WinDbg (Local Kernel) | Kernel structure state |

---

## Scene 1 — "EDR Is Running"

```cmd
sc start EDR_SIM
sc start FILTER_SIM
```

**DebugView++ output:**

```
[EDR_SIM] Process callback registered — EDR watching
[FILTER_SIM] Minifilter registered — watching all files
```

**Process Hacker:** Both drivers listed as Running.

Now open Notepad. Open CMD. Access a file.  
Watch DebugView++ log every action in real time.

> This is the baseline. Full visibility. Both hooks active.

---

## Scene 2 — "Attacker Loads a Driver"

```cmd
sc create HelloDriver type= kernel binPath= C:\HelloDriver.sys
sc start HelloDriver
```

**DebugView++ output:**

```
[EDR_SIM] New process created: sc.exe (PID: 4521)
[HelloDriver] Hello from Ring 0!
```

**What to observe:**  
EDR_SIM sees `sc.exe` launch — because that callback is still active.  
But `sc.exe` is a legitimate Windows binary. No block. No alert.  
The driver is now in Ring 0. The IOCTL channel is open.

**Process Hacker:** Three drivers now running — EDR_SIM, FILTER_SIM, HelloDriver.

---

## Scene 3 — "Callbacks Killed"

Simulate the IOCTL-driven callback removal by unloading EDR_SIM:

```cmd
sc stop EDR_SIM
```

**DebugView++ output:**

```
[EDR_SIM] Callback unregistered — EDR blind now
```

Now open Notepad. Open CMD. Open a browser.

**DebugView++ output:**

```
(nothing from EDR_SIM)
[FILTER_SIM] File access: \...\notepad.exe
```

Process visibility — gone.  
File visibility — still active (minifilter not touched yet).

---

## Scene 4 — "Minifilter Killed"

```cmd
sc stop FILTER_SIM
```

**DebugView++ output:**

```
[FILTER_SIM] Minifilter unregistered — file ops invisible
```

Now access files, launch processes, do anything.

**DebugView++ output:**

```
(nothing)
```

Complete silence.  
Process callbacks — gone.  
Minifilter — gone.  
HelloDriver is still loaded in Ring 0.

---

## Scene 5 — "Operate Freely"

Try to open `mimikatz.exe` (renamed empty file from Simulation 04).

**DebugView++ output:**

```
(nothing)
```

It opens. No block. No log. No alert.

Open Process Hacker. Look at the Services tab.  
The `EDR_SIM` and `FILTER_SIM` entries are there — state shows Stopped.  
`HelloDriver` is Running.

> In a real scenario: the EDR process would still show as Running in Ring 3.  
> Its service would be healthy. Its dashboard would be green.  
> But its kernel hooks are gone. It cannot see anything.

---

## Scene 6 — "The Forensic Trail"

Open Event Viewer:

```
Windows Logs → System → Filter: Event ID 7045
```

You will see every driver load from this entire lab session:

```
Event ID 7045 — A new service was installed
Service Name: HelloDriver
Service File Name: C:\HelloDriver.sys
Service Type: kernel mode driver

Event ID 7045 — A new service was installed
Service Name: EDR_SIM
...

Event ID 7045 — A new service was installed
Service Name: FILTER_SIM
...
```

**What this means for defenders:**  
Event ID 7045 is the forensic footprint of every driver load.  
This is the signal that exists *before* the damage is done.  
Alerting on unexpected 7045 events — especially from unusual paths — is the detection opportunity.

---

## Full Sequence Summary

| Scene | What Happened | Visibility State |
|---|---|---|
| 1 | EDR callbacks and minifilter loaded | Full — every process and file logged |
| 2 | Attacker driver loaded | Full — sc.exe seen, but not blocked (legitimate binary) |
| 3 | Process callbacks removed | Partial — file ops still visible, processes invisible |
| 4 | Minifilter removed | Zero — nothing visible |
| 5 | Operate freely | No logs, no blocks, no alerts |
| 6 | Event Viewer | 7045 entries logged for every driver load |

---

## Observed Output to Screenshot

- All four windows visible simultaneously — Scene 2 moment (three drivers loaded, EDR still active)
- DebugView++ before and after Scene 3 — side by side comparison
- Event Viewer showing 7045 entries for each driver loaded during the lab

---

## What the Lab Proved End to End

- [ ] Ring 3 and Ring 0 are physically separated — confirmed with WinDbg addresses and failed memory read
- [ ] A driver loads into kernel address space — confirmed with `lm m` and `FFFF...` addresses in debug output
- [ ] IOCTL is the communication bridge — both sides of the conversation observed simultaneously
- [ ] Callbacks fire on every kernel event — and go completely silent when removed
- [ ] Minifilter intercepts every file operation — and becomes invisible when unregistered
- [ ] Event ID 7045 is the one signal that exists throughout the entire sequence