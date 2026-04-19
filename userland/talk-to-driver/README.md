# Talk To Driver

User-mode client that communicates with `IoctlDriver` via IOCTL.

**Purpose in this lab:**  
The Ring 3 side of the IOCTL conversation.  
Run this alongside DebugView++ to watch both ends of the channel simultaneously.

---

## What It Does

1. Opens a handle to `\\.\IoctlDriver`
2. Sends `IOCTL_HELLO` — receives a string reply from Ring 0
3. Sends `IOCTL_ADD` with two integers — receives computed result from Ring 0
4. Prints both exchanges to the console

---

## How to Compile

Inside the VM, open Developer Command Prompt for Visual Studio:

```cmd
cl TalkToDriver.cpp /Fe:TalkToDriver.exe
```

Or create a new Console Application in Visual Studio, paste the source, build x64.

---

## How to Run

`IoctlDriver` must be loaded first. Then:

```cmd
TalkToDriver.exe
```

---

## Expected Console Output

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

---

## Run Alongside

- DebugView++ → see Ring 0 side of the same conversation
- Process Hacker → confirm driver handle is open during execution

---

## Used In

- `docs/03-ioctl-communication.md`
- `docs/06-full-picture.md`