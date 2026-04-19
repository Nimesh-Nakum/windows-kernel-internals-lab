# 01 — Ring 3 vs Ring 0: See the Boundary

**Tools:** Process Hacker 2, WinDbg, PowerShell  
**Time:** ~20 minutes  
**Snapshot before starting**

---

## Step 1 — See the Separation in Process Hacker

Open Process Hacker as Administrator.  
Right-click any column header → enable:
- Integrity Level
- Session
- Description

Look at what you have:

```
notepad.exe      Integrity: Medium    → Ring 3
chrome.exe       Integrity: Medium    → Ring 3
svchost.exe      Integrity: System    → Ring 3 (SYSTEM account — not Ring 0)
```

Now go to: **Services tab → filter Type = Kernel Driver**

```
ntfs.sys         Kernel Driver        → Ring 0
NDIS.sys         Kernel Driver        → Ring 0
WdFilter.sys     Kernel Driver        → Ring 0  ← Windows Defender lives here
```

**What you are seeing:**  
User processes and kernel drivers are two completely different worlds.  
`WdFilter.sys` — the Windows Defender minifilter — is in Ring 0.  
Your terminal is in Ring 3. They do not share memory.

---

## Step 2 — See Kernel Objects in WinDbg

Open WinDbg as Administrator → **File → Kernel Debug → Local**

Run:
```
!process 0 0
```

Pick any process from the list. Copy its address. Run:
```
!process <address> 7
```

You will see something like this:
```
PROCESS ffff9a8b12345678        ← kernel address — this is Ring 0
    SessionId: 1
    Cid: 1234                   ← PID you recognize from Task Manager
    Token: ffffa08b98765432     ← security token sitting in kernel memory
    VirtualSize: 2097152 KB     ← Ring 3 address space
    THREAD ffff9a8b11223344     ← thread object in kernel
```

**What you are seeing:**  
Every process you see in Task Manager has a `_EPROCESS` structure in Ring 0.  
`ffff...` addresses are kernel space. Ring 3 cannot touch them directly.

---

## Step 3 — Try to Read Kernel Memory From Ring 3 (Watch It Fail)

Open a standard PowerShell — not admin. Paste this:

```powershell
$sig = @"
[DllImport("ntdll.dll")]
public static extern int NtReadVirtualMemory(
    IntPtr ProcessHandle, IntPtr BaseAddress,
    byte[] Buffer, uint NumberOfBytesToRead,
    ref uint NumberOfBytesRead);
"@
$ntdll = Add-Type -MemberDefinition $sig -Name "NtDll" -Namespace "Win32" -PassThru

$buffer = New-Object byte[] 8
$read = [uint32]0
$result = [Win32.NtDll]::NtReadVirtualMemory(
    [IntPtr]-1,
    [IntPtr]0xFFFF800000000000,
    $buffer, 8, [ref]$read)

Write-Host "Result: $result"
```

**Expected output:**
```
Result: -1073741819    ← STATUS_ACCESS_VIOLATION
```

**What you are seeing:**  
A hard stop. Not a permission dialog — the CPU itself rejected the read.  
Ring 3 has no path to kernel memory without something in Ring 0 carrying it across.  
That is the entire reason a driver is needed as a proxy.

---

## Observed Output to Screenshot

- Process Hacker with Integrity Level column showing driver vs process separation
- WinDbg `!process` output with `ffff...` kernel addresses visible
- PowerShell showing `STATUS_ACCESS_VIOLATION`

---

**Next → [02-driver-loading.md](./02-driver-loading.md)**