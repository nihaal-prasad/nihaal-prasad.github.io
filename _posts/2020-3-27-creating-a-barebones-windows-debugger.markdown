---
layout: post
title: "Creating a Barebones Windows Debugger"
date: 2020-3-27
---

Today, I will be showing how I created a basic debugger in C for Windows programs. This will be very similar to the <a href="https://nihaal-prasad.github.io/2020/03/07/creating-a-barebones-linux-debugger.html">Linux Debugger</a> that I made a while back, but will be quite different because I will be using the Windows debugging API as opposed to the Linux one.

```
static PROCESS_INFORMATION pi = {0}; // Contains information about the debugged process
static int dwContinueStatus = DBG_CONTINUE; // The status for continuing execution
static char cont = 1; // This is set to 0 when the debugger exits

int main(int argc, char** argv) {
    // Initialize some variables
    STARTUPINFO si; // Contains startup information about the debugged process
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create the process to debug
    CreateProcessA(argv[1], NULL, NULL, NULL, 0, DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi);

    // Process debugging events
    DEBUG_EVENT debug_event = {0};
    while(cont) {
        if(!WaitForDebugEvent(&debug_event, INFINITE)) {
            break; // Break the loop if the function fails
        }
        ProcessDebugEvent(debug_event); // User-defined function that will process the event
        ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, dwContinueStatus); // Continue execution
    }

    // Exit the debugger
    printf("Debugger will now exit.\n");
    return 0;
}
```

The first thing that we need to do is create our main() function, which will create a new child process for debugging. We will do this by using the <a href="https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa">CreateProcessA()</a> function, which will allow us to create a process for the program that we wish to debug. This function requires us to pass in two pointers: one pointing to a <a href="https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-startupinfoa">STARTUPINFO struct</a> and another pointing to a <a href="https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-process_information">PROCESS_INFORMATION struct</a>. Note that the PROCESS_INFORMATION variable is a global variable because we will be using the values in it inside of other functions. The CreateProcessA() function will fill up these two structs with information about the process that we just created.

Our actual debugger will be executed inside of a while loop, and a global variable called "cont" (one by default) will be set to zero whenever we want to stop the debugger. Whenever certain types of events occurs (such as hitting a breakpoint, for example), the child process pauses execution and notifies the debugger that something called a <a href="https://docs.microsoft.com/en-us/windows/win32/debug/debugging-events">debugging event</a> has occurred. Our debugger must continuously wait for debugging events to occur by using the <a href="https://docs.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent">WaitForDebugEvent()</a> function so that we can take the appropriate action. Note that the WaitForDebugEvent() function returns zero if some kind of error occurs, in which case, we will immidiately stop the debugger. We are required to pass in a pointer to a <a href="https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-debug_event">DEBUG_EVENT</a> struct to the WaitForDebugEvent() function, which will automatically fill up the struct with information about the debugging event.

Once some debug event occurs, we will process the event by using the user-defined function ProcessDebugEvent(), whose code I will show a little further below. This function will select the appropriate action for a particular debug event (such as dealing with an exception). After we have finished processing the debugger, we will then continue the debuggee's execution by calling <a href="https://docs.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-continuedebugevent">ContinueDebugEvent()</a>, which is equivalent to using the "continue" command in GDB. The dwContinueStatus variable contains information for how we should be continuing execution, and it should be changed to DBG_EXCEPTION_NOT_HANDLED when we are dealing with breakpoints (more on this later).

```
typedef struct _DEBUG_EVENT {
  DWORD dwDebugEventCode;
  DWORD dwProcessId;
  DWORD dwThreadId;
  union {
    EXCEPTION_DEBUG_INFO      Exception;
    CREATE_THREAD_DEBUG_INFO  CreateThread;
    CREATE_PROCESS_DEBUG_INFO CreateProcessInfo;
    EXIT_THREAD_DEBUG_INFO    ExitThread;
    EXIT_PROCESS_DEBUG_INFO   ExitProcess;
    LOAD_DLL_DEBUG_INFO       LoadDll;
    UNLOAD_DLL_DEBUG_INFO     UnloadDll;
    OUTPUT_DEBUG_STRING_INFO  DebugString;
    RIP_INFO                  RipInfo;
  } u;
} DEBUG_EVENT, *LPDEBUG_EVENT;
```

Before I show you the code for ProcessDebugEvent(), I need to explain how DEBUG_EVENT stores information. Essentially, there is a value inside of the struct called dwDebugEventCode, which contains a number indicating what kind of debugging event happened. The union variable u will then be filled up with the appropriate information relative for each kind of debug event (for example, if an exception occurs, then we could access information about the exception by using debug_event.u.Exception). This makes it easy for us to figure out what the correct action to take is when we receive a debugging event when dealing with it inside of ProcessDebugEvent().

```
void ProcessDebugEvent(DEBUG_EVENT debug_event) {
    // Reset the continue status (in case it was changed while processing an exception)
    dwContinueStatus = DBG_CONTINUE;

    // Call the correct function depending on what the event code is
    switch(debug_event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT: // Called when the debuggee process is first created
            ProcessCreation(debug_event);
            break;
        case OUTPUT_DEBUG_STRING_EVENT: // Called when a string is sent to the debugger for display
            OutputString(debug_event);
            break;
        case EXCEPTION_DEBUG_EVENT: // Called whenever any exception occurs in the process being debugged
            ProcessException(debug_event);
            break;
        case EXIT_PROCESS_DEBUG_EVENT: // Called when the debuggee process exits
            ExitDebuggeeProcess(debug_event);
            break;
    }
}
```

As you can see here, there are four main debugging events that we will be dealing with (there are more, but these are the only ones we need for creating a basic debugger). Each of these events will call a different user-defined function, which will take appropriate action for dealing with that case. I will start by explaining the simplest one, which is the ExitDebuggeeProcess() function.

```
// Called when the debuggee exits
void ExitDebuggeeProcess(DEBUG_EVENT debug_event) {
    printf("Process exited with code %d (0x%x).\n", debug_event.u.ExitProcess.dwExitCode, debug_event.u.ExitProcess.dwExitCode);
    cont = 0; // Stop the debugger
}
```

The function is called whenever we receive an EXIT_PROCESS_DEBUG_EVENT, which as you can probably guess by the name, indicates that the debuggee has exited. The function code is simple: we just tell the user what the exit code is (both in decimal and in hex), and then we set cont to zero, which will stop the while loop that we had created in main.

```
// Allocates memory on the heap
void *mymalloc(int size) {
    void *mem = malloc(size);
    if(mem == NULL) {
        printf("Error allocating memory on the heap.");
        exit(0);
    }
    return mem;
}

// Called when the debuggee outputs a debug string
void OutputString(DEBUG_EVENT debug_event) {
    // Obtains information (including a pointer) about the string being printed
    // Note that this pointer is only valid on the debuggee's process, but not on the debugger's process
    // So we'll have to read from the debuggee's process and copy that string's value into a string in our process
    OUTPUT_DEBUG_STRING_INFO DebugString = debug_event.u.DebugString;

    // Create space on the heap to store the string being printed
    char* str = mymalloc(DebugString.nDebugStringLength);

    // Read the string from the debuggee's memory and print it
    ReadProcessMemory(pi.hProcess, DebugString.lpDebugStringData, str, DebugString.nDebugStringLength, NULL);
    printf("Debug String Received: %s\n", str);

    // Free the heap
    free(str);
    str = NULL;
}
```

The other simple function is executed when we receive OUTPUT_DEBUG_STRING_EVENT, which is thrown if the debuggee process sends the debugger process a debug string. This can happen if the debuggee calls a function such as <a href="https://docs.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-outputdebugstringa">OutputDebugStringA()</a>, which will indicate to the system that the debuggee wants to send a string to the debugger. This debug event will set debug_event.u.DebugString equal to a pointer to the string that needs to be printed out. There is one problem with this pointer: it points to a memory address that is on another process and therefore won't directly work as a string. We can deal with this problem by using the <a href="https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory">ReadProcessMemory()</a> function, which allows us to copy bytes from another process into an array in our process. From there, it is easy to just print out the string that was received and move on.

```
static CREATE_PROCESS_DEBUG_INFO pInfo = {0}; // Contains information about the process creation

// Called when the debuggee process is being created
void ProcessCreation(DEBUG_EVENT debug_event) {
    // Obtain information about the process's creation
    pInfo = debug_event.u.CreateProcessInfo;

    // Add a breakpoint at the start address
    printf("Setting a breakpoint at the start address...\n");
    AddBreakpoint(pInfo.lpStartAddress);
}
```

The ProcessCreation() function is called when we receive a CREATE_PROCESS_DEBUG_EVENT, which is always received by the debugger right before the debuggee first begins execution. The <a href="https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-create_process_debug_info">CREATE_PROCESS_DEBUG_INFO struct</a> will contain important information about the process that we will need to access later. This information is given to us via debug_event.u.CreateProcessInfo. One of the pieces of data that the CREATE_PROCESS_DEBUG_INFO struct will contain is the start address of the program. It is vital for us to set a breakpoint at the start address so that the person using the debugger can run commands at that point. Note that the start address of a program is not necessarily the first line of code the debuggee process will execute; there is usually some overhead code that is executed even before we get to start executing the start address.

Before I show you the code for AddBreakpoint(), I need to show you the basic idea behind setting breakpoints. The "INT 3" assembly instruction (0xCC in hex) will generate a system interrupt that will tell the operating system that a debugging event has occurred. More specifically, once a process reads the instruction 0xCC, it will tell the debugger that an exception has occurred, and the debug event associated with that exception will indicate to the debugger that we have hit a breakpoint. These are the steps that we need to do in order to setup a breakpoint:
<ol>
    <li>Figure out which line we want to set the breakpoint.</li>
    <li>Read the first byte at the address where we want to set the breakpoint.</li>
    <li>Store the address of the line and the original value of the first byte in a linked list.</li>
    <li>Replace the first byte of the address where we want to set the breakpoint with 0xCC.</li>
    <li>Continue execution and wait until the debuggee has hit the breakpoint.</li>
    <li>Push RIP backwards by one byte (RIP would have moved forward as soon as it read INT 3).</li>
    <li>Search the linked list until we find a node with its address equal to RIP and obtain the original value of the first byte.</li>
    <li>Delete the node from the linked list.</li>
    <li>Change the first byte of the address back to its original value (which would've been saved in the linked list).</li>
</ol>

Make sure you understand these steps before moving on.

```
// Contains information for a single breakpoint (used as a linked list)
typedef struct _breakpoint {
    char byte; // Contains the byte that will be overwritten with INT 3
    struct _breakpoint *next; // Contains the next value in the linked list
    void *addr; // Contains the address that the breakpoint is at
} Breakpoint;
static Breakpoint *head = NULL; // Head of the linked list of breakpoints

// Adds a breakpoint to the linked list of breakpoints
void AddBreakpoint(void *addr) {
    // Create space on the heap for this breakpoint
    Breakpoint *b = mymalloc(sizeof(Breakpoint));
    b->addr = addr;

    // Get the byte that we want to replace with INT 3 and store it in b.byte
    ReadProcessMemory(pInfo.hProcess, addr, &(b->byte), 1, NULL);

    // Insert an INT 3 (0xCC) instruction
    char byte = 0xCC;
    WriteProcessMemory(pInfo.hProcess, addr, &byte, 1, NULL);
    FlushInstructionCache(pInfo.hProcess, addr, 1);

    // Insert this into the linked list
    b->next = head;
    head = b;
}
```

Our struct for breakpoints needs to store three values: the original value of the byte that will be overwritten with INT 3, the next value in the linked list, and the address of the breakpoint. The AddBreakpoint() function is used to deal with steps one through four. It starts off by allocating memory for the breakpoint and saving the address of that breakpoint. We then use ReadProcessMemory() to store the value of the very first byte into b->byte. Then we use <a href="https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-writeprocessmemory">WriteProcessMemory()</a> to overwrite the byte that we just read with INT 3. Because it is possible that the CPU has already started to load the next instruction, we must call <a href="https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-flushinstructioncache">FlushInstructionCache()</a> to ensure that our changes are made. Finally, we add this breakpoint to the linked list.

```
// Called when the debuggee receives an exception
void ProcessException(DEBUG_EVENT debug_event) {
    // Look at the status for the exception
    int code = debug_event.u.Exception.ExceptionRecord.ExceptionCode;
    switch(code) {
        case STATUS_BREAKPOINT: // Called when the exception was caused by a breakpoint
            ProcessBreakpoint(debug_event);
            break;
        default:
            printf("Exception %d (0x%x) received.\n", code, code);
            ProcessCommands(); // Allow the user to type in commands into the debugger
            break;    
    }
}
```

Our next function, ProcessException(), is used to deal with step five, and it is called whenever EXCEPTION_DEBUG_EVENT is received. If the exception code for the exception indicates that we have hit a breakpoint, then we will call ProcessBreakpoint() to deal with this breakpoint. Else, we will just print out the exception number in both decimal and hex. The ProcessCommands() function will be shown a little later, and it will be used to allow the user to type commands into the debugger.

```
void ProcessBreakpoint(DEBUG_EVENT debug_event) {
    if(head != NULL) { // Do nothing if the head of the breakpoint linked list is NULL
        // Get the value of RIP
        CONTEXT lcContext;
        lcContext.ContextFlags = CONTEXT_ALL;
        GetThreadContext(pInfo.hThread, &lcContext); // Obtains the thread context (which contains info about registers)
        lcContext.Rip--; // Move RIP back one byte (RIP would've moved forward as soon as it read INT 3)

        // Find the breakpoint in the linked list, obtain the byte that was originally there and its address, and delete the node from the linked list
        char byte = 0;
        void *addr = NULL;
        char found = 1; // This is set to zero if we did not find the correct byte
        if(head->addr == (void *) lcContext.Rip) { // Triggered if the head is the breakpoint we're looking for
            byte = head->byte; // Save the byte
            addr = head->addr; // Save the address

            // Delete the head
            Breakpoint *del = head;
            head = head->next;
            free(del);
        } else { // Else, loop until we find the correct breakpoint
            Breakpoint *b = head;
            while(b->next != NULL && b->next->addr != (void *) lcContext.Rip) {
                b = b->next;
            }
            if(b->next != NULL) {
                byte = b->next->byte; // Save the byte
                addr = b->next->addr; // Save the address

                // delete the correct node
                Breakpoint *del = b->next;
                b->next = del->next;
                free(del);
            } else { // If this else statement hits, then we did not find the breakpoint in the linked list, and we will just ignore it
                found = 0;
            }
        }
        if(found) {
            // Indicate that we have hit a breakpoint
            dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED; // The debuggee should not handle this exception
            printf("Hit a breakpoint!\n");

            // Apply the change to RIP (which was moved one byte backwards earlier)
            SetThreadContext(pInfo.hThread, &lcContext);

            // Replace the INT 3 instruction with the byte that was originally there
            WriteProcessMemory(pInfo.hProcess, addr, &byte, 1, NULL);
            FlushInstructionCache(pInfo.hProcess, addr, 1);

            // Allow the user to type in commands into the debugger
            ProcessCommands();
        }
    }
}
```

This next function is long, but it will allow us to deal with steps six through nine for dealing with breakpoints. First we will check whether the head is NULL, and if it is NULL, then we will do nothing. We must do this check because usually the operating system automatically sets a breakpoint at the start of wherever the process first starts executing (which, by the way, is not necessarily the start address because there is some overhead involved with execution), and we do not want to do anything with this first breakpoint.

If the check passes, then we will get the value of RIP by using <a href="https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext">GetThreadContext()</a>, which requires a pointer to a <a href="https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-context">CONTEXT</a> struct. The CONTEXT struct is used to store information about a thread's context, and it should contain the values of various registers. Once we obtain Rip from the CONTEXT struct, we must subtract it by one to move it backwards (but we won't apply these changes to the debuggee until once we are sure that the current address is inside of the breakpoint linked list).

The next step is to find the node in the linked list that contains an addr value that is equal to the current value of RIP. Once we find the correct node, we will save the value of the original byte, and we will save the value of addr. We can delete the node from the linked list of breakpoints after that. The code for doing this should be simple to understand if you understand how linked lists work.

If we have found the node in the linked list, then there are a few more steps that we need to take care of. First, we must set dwContinueStatus equal to DBG_EXCEPTION_NOT_HANDLED, which will tell the system that the debuggee should not handle this exception. Next, we have to apply our changes to RIP by calling <a href="https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext">SetThreadContext()</a> with our modified context as a parameter. We can then replace the INT 3 instruction with the byte that was originally there by using WriteProcessMemory() and FlushInstructionCache(). Finally, we will allow the user to type in commands into the debugger by calling the ProcessCommands() function.

```
// Allows the user to type in commands into the debugger
void ProcessCommands() {
    char *cmd = mymalloc(200); // The command that the user types in
    while(strncmp(cmd, "continue", 8) != 0 && strncmp(cmd, "cont", 4) != 0) {
        printf("> ");
        fgets(cmd, 200, stdin); // Read a line

        if(strncmp(cmd, "registers", 9) == 0 || strncmp(cmd, "regs", 4) == 0) {
            PrintRegs(); // Prints out all of the values of the registers
        } else if(strncmp(cmd, "break ", 6) == 0 || strncmp(cmd, "b ", 2) == 0) {
            strtok(cmd, " "); // The value after the space should be the address in hex
            AddBreakpoint((void *) strtoll(strtok(NULL, " "), 0, 16)); // Adds a breakpoint at that address
        } else if(strncmp(cmd, "mem ", 4) == 0) {
            strtok(cmd, " ");
            char *a = strtok(NULL, " "); // The value after the first space should be the address in hex
            char *b = strtok(NULL, " "); // The value after the second space should be the number of bytes to read in decimal
            ReadMemory(a, atoi(b)); // Read from the given memory address
        } else if(strncmp(cmd, "quit", 4) == 0 || strncmp(cmd, "q", 1) == 0 || strncmp(cmd, "exit", 4) == 0) {
            printf("Debugger will now exit.\n"); // Exit the program
            exit(0);
        } else if(strncmp(cmd, "help", 4) == 0) {
            printf("continue: Continues execution.\n");
            printf("registers: Prints out the values of all of the registers.\n");
            printf("break <addr>: Sets a breakpoint at a given address.\n");
            printf("mem <addr> <bytes>: Reads a given number of bytes from a given memory address.\n");
            printf("quit: Closes the debugger.\n");
        }
    }
    
}
```

The ProcessCommands() function has six commands: help, continue, registers, break, mem, and quit. The help command shows more details for what each command does and what their parameters are. This function is mainly used to process input that the user types in and select the correct function to call.

```
// Prints out all of the values of the registers
void PrintRegs() {
    // Read the registers
    CONTEXT lcContext;
    lcContext.ContextFlags = CONTEXT_ALL;
    GetThreadContext(pInfo.hThread, &lcContext);
    
    // Print out all of the values of the registers
    printf("RAX: 0x%llx\n", lcContext.Rax);
    printf("RBX: 0x%llx\n", lcContext.Rbx);
    printf("RCX: 0x%llx\n", lcContext.Rcx);
    printf("RDX: 0x%llx\n", lcContext.Rdx);
    printf("RSP: 0x%llx\n", lcContext.Rsp);
    printf("RBP: 0x%llx\n", lcContext.Rbp);
    printf("RSI: 0x%llx\n", lcContext.Rsi);
    printf("RDI: 0x%llx\n", lcContext.Rdi);
    printf("R8: 0x%llx\n", lcContext.R8);
    printf("R9: 0x%llx\n", lcContext.R9);
    printf("R10: 0x%llx\n", lcContext.R10);
    printf("R11: 0x%llx\n", lcContext.R11);
    printf("R12: 0x%llx\n", lcContext.R12);
    printf("R13: 0x%llx\n", lcContext.R13);
    printf("R14: 0x%llx\n", lcContext.R14);
    printf("R15: 0x%llx\n", lcContext.R15);
    printf("RIP: 0x%llx\n", lcContext.Rip);
}
```

PrintRegs() is used to print out all of the values of the registers. I had already explained how to read the value of RIP when I went over how ProcessBreakpoint() worked, so reading the values of all of the registers should be pretty self-explanatory by this point.

```
// Reads n bytes from the given memory address
void ReadMemory(char *addr_hex, int n) {
    // Convert the address from a hex string into a DWORD64
    long long addr = strtoll(addr_hex, 0, 16);
    printf("Reading memory from address 0x%llx...\n", addr);

    // Read n bytes from the given memory address
    char *buf = mymalloc(n);
    ReadProcessMemory(pInfo.hProcess, (LPCVOID) addr, buf, n, NULL);

    // Loop through each byte in the buffer and print it out
    for(int i = 0; i < n; i++) {
        printf("0x%x ", buf[i]);
    }
    printf("\n");
}
```

The last function that I need to go over is ReadMemory(). I had already gone over how ReadProcessMemory() worked when I went over how OutputString() worked, so this should also be self-explanatory by this point.

We are finally done building our debugger! You can see the entire code <a href="https://github.com/nihaal-prasad/Barebones-Windows-Debugger/blob/master/debugger.c">here</a> if you want to see it all at once.

{% include related_posts.html %}
