# Lab Observations

Use this file to record what you see during each simulation.  
Paste output, note anything unexpected, and track what matched or differed from the expected results.

---

## Simulation 01 — Ring 3 vs Ring 0

**Date:**  
**VM Snapshot name:**

### Process Hacker — What I Saw

```
(paste your driver list here)
```

### WinDbg — _EPROCESS Output

```
(paste your !process output here)
```

### PowerShell — Memory Read Result

```
(paste the STATUS_ result here)
```

### Notes

```
(anything unexpected, any errors, observations)
```

---

## Simulation 02 — Driver Loading

**Date:**  
**VM Snapshot name:**

### DebugView++ Output

```
(paste kernel output here)
```

### WinDbg lm m HelloDriver Output

```
(paste address range here)
```

### Notes

```

```

---

## Simulation 03 — IOCTL Communication

**Date:**  
**VM Snapshot name:**

### Ring 3 Console Output (TalkToDriver.exe)

```
(paste here)
```

### Ring 0 DebugView++ Output (simultaneous)

```
(paste here)
```

### Notes

```

```

---

## Simulation 04 — Callbacks

**Date:**  
**VM Snapshot name:**

### DebugView++ — Callback Active (process launches)

```
(paste output — should show every process name)
```

### DebugView++ — After sc stop EDR_SIM

```
(should be empty — paste nothing or confirm silence)
```

### mimikatz.exe Test — Before and After

```
Before sc stop: (blocked / allowed?)
After sc stop:  (blocked / allowed?)
```

### Notes

```

```

---

## Simulation 05 — Minifilters

**Date:**  
**VM Snapshot name:**

### fltMC Output — Before Loading FILTER_SIM

```
(paste full filter stack)
```

### fltMC Output — After Loading FILTER_SIM

```
(paste updated stack — confirm your filter appears)
```

### DebugView++ — File Operations While Active

```
(paste a sample — 5-10 lines of file paths)
```

### DebugView++ — After sc stop FILTER_SIM

```
(confirm silence)
```

### Notes

```

```

---

## Simulation 06 — Full Picture

**Date:**  
**VM Snapshot name:**

### Scene-by-Scene DebugView++ Output

**Scene 1 (both loaded):**
```

```

**Scene 2 (attacker driver loaded):**
```

```

**Scene 3 (process callbacks killed):**
```

```

**Scene 4 (minifilter killed):**
```

```

**Scene 5 (free operation):**
```

```

### Event Viewer — 7045 Entries

```
(list the driver names that appeared in 7045 events)
```

### What Surprised Me

```

```

### What I Would Alert On

```

```

---

## General Notes

```
(anything across all simulations — patterns noticed, questions, things to research further)
```