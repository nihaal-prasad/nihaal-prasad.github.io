---
layout: post
title: "Creating a Barebones Debugger"
date: 2020-3-7
---

Today, I will be showing how I created a basic debugger in C. This won't be a very complicated debugger as it will only allow you to do three things: see the values of the registers, step through one instruction, and read hex values from memory. Obviously, this is nothing compared to something like IDA Pro, but making a basic debugger did help me understand what the debugger is actually doing when we are using it.

The first thing that we need to do is to run the program we want to debug in a separate process using fork(). The program's name will be given as a command line argument to our debugger, and any additional arguments that we want to past to the program we are currently debugging can also be passed as arguments. Once we call fork() to make a copy of this process, we will make the child process call `ptrace(PTRACE_TRACEME, 0, 0, 0);`, which will let the child process know that it is going to be debugged by the parent process. The `ptrace()` function will be used throughout our code as it allows the parent process to control and observe the child process. From there, we will use execv() to actually execute the program that we want to execute. As soon as this happens, the tracee (the program being debugged) stop pause execution on a SIGTRAP, which will be used to prevent the tracee from executing any more code unless the tracer/debugger allows it.

```
// Represents the PID of the program being debugged
static int pid = 0;

// Forks this process, runs the given executable using the given arguments, and returns the PID to the parent
int run(char **argv) {
    pid = fork();

    // If statement will only execute if we are the child process
    if(pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0); // Allow the parent to trace us
        execv(argv[1], &(argv[2])); // Execute the program we wanted to execute.
        // The tracee should stop on SIGTRAP as soon as execv() is called.
        // This will prevent the tracee from executing any code unless the tracer allows it to using ptrace()
    }
}
```

Now, the child process has the program that we want to debug loaded up into it, and it has paused execution so that we can start to take control of it as its parent process. Inside our main function, we need to obtain the program that we must run from the command line arguments and call run(). Then we will also use `ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);` on the parent process so that if the parent exits, then the child process must also exit.

```
// Main function
int main(int argc, char **argv) {
    // Make sure we have the correct number of arguments
    if(argc != 2) {
        printf("Usage: ./debug <program> <additional arguments>\n");
        exit(0);
    }

    run(argv); // Run the given executable and obtain its PID
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL); // Make sure that if the tracer exits, then the tracee must also exit
    debugger(pid); // Run the debugger
}
```

The debugger() function is a function that I made to help me process commands inputted by the user. It will parse the command the user types in and calls the correct function to do the action that the user requested. I also made a print_help() function that will print out all of the possible commands that can be used.

```
// Reads a line of input
void readline(char *input) {
    // Reads a line of input
    char buf[500];
    printf("> ");
    fgets(input, 500, stdin);

    // Get rid of the '\n' character
    int len = strlen(input);
    if(len >= 2 && input[len-1] == '\n') {
        input[len-1] = '\0';
    }
}

// Prints the help menu
void print_help() {
    printf("regs: Prints out the values of each register.\n");
    printf("step: Executes a single instruction.\n");
    printf("mem <addr>: Reads the value at the given memory address.\n");
    printf("quit: Closes the debugger.\n");
}

// Actual debugger code
void debugger() {
    char *input = malloc(500); // Input that the user types in

    // Loop until the user types in "exit"
    do {
        readline(input); // Read a command

        // Parse the command
        if(strncmp("regs", input, 5) == 0 || strncmp("r", input, 2) == 0) {
            print_regs();
        } else if(strncmp("step", input, 5) == 0 || strncmp("s", input, 2) == 0) {
            step();
        } else if(strncmp("mem ", input, 4) == 0) {
            strtok(input, " "); // Mem should have a single input parameter, which is the memory address to read in hex
            read_mem(strtok(NULL, " "));
        } else if(strncmp("help", input, 5) == 0 || strncmp("h", input, 2) == 0) {
            print_help();
        }
    } while(strncmp(input, "exit", 5) && strncmp(input, "quit", 5) && strncmp(input, "q", 2));
}
```

The first thing that I will show you how to do is how to read the values of the registers in the debugged program. We will be using the `ptrace(PTRACE_GETREGS, pid, 0, &regs)` function to read the values of the registers in the debugged program. The "regs" variable that we'll use as an input parameter for this function has a type of `struct user_regs_struct`, and the ptrace() function will store the values of the registers inside of that variable. The values of the registers can then be easily read by using the regs variable (i.e. use `regs.rax` to read the value of rax).

```
// Prints out all of the register values inside of the process we're debugging
void print_regs() {
    // Create the struct that will contain all of the process' registers
    struct user_regs_struct regs;

    // Read the values of each register and store it inside of the struct
    ptrace(PTRACE_GETREGS, pid, 0, &regs);

    // Print out each value
    printf("rip 0x%llx\n", regs.rip);
    printf("rax 0x%llx\n", regs.rax);
    printf("rbx 0x%llx\n", regs.rbx);
    printf("rcx 0x%llx\n", regs.rcx);
    printf("rdx 0x%llx\n", regs.rdx);
    printf("rsi 0x%llx\n", regs.rsi);
    printf("rdi 0x%llx\n", regs.rdi);
    printf("rsp 0x%llx\n", regs.rsp);
    printf("rbp 0x%llx\n", regs.rbp);
    printf("r8 0x%llx\n", regs.r8);
    printf("r9 0x%llx\n", regs.r9);
    printf("r10 0x%llx\n", regs.r10);
    printf("r11 0x%llx\n", regs.r11);
    printf("r12 0x%llx\n", regs.r12);
    printf("r13 0x%llx\n", regs.r13);
    printf("r14 0x%llx\n", regs.r14);
    printf("r15 0x%llx\n", regs.r15);
}
```

To read any word from memory, we can use `ptrace(PTRACE_PEEKDATA, pid, addr, 0)`, where addr is a long representing the address that we want to read from. This function will return a long variable that we can then print out in hex.

```
// Reads the value at the given memory address
void read_mem(char *addr_input) {
    // Make sure that the input isn't NULL
    if(addr_input != NULL) {
        long addr = strtol(addr_input, 0, 16); // Convert the address from a hex string to a long
        long data = ptrace(PTRACE_PEEKDATA, pid, addr, 0); // Obtain the data
        printf("%lx\n", data);
    }
}
```

Stepping through a single line of assembly code is even easier. We can just call `ptrace(PTRACE_SINGLESTEP, pid, 0, 0);`, which will just execute one line of code.
```
// Steps through a single line of assembly code
void step() {
    ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
}

```

Now we should have a working debugger. You can try using this to debug a basic program, and it should work just fine.
```
$ ./debug todebug
> help
regs: Prints out the values of each register.
step: Executes a single instruction.
mem <addr>: Reads the value at the given memory address.
quit: Closes the debugger.
> regs
rip 0x7fcfb2eee090
rax 0x0
rbx 0x0
rcx 0x0
rdx 0x0
rsi 0x0
rdi 0x0
rsp 0x7fff2e69c670
rbp 0x0
r8 0x0
r9 0x0
r10 0x0
r11 0x0
r12 0x0
r13 0x0
r14 0x0
r15 0x0
> mem 0x7fcfb2eee090
f98e8e78948
> step
> r
rip 0x7fcfb2eee093
rax 0x0
rbx 0x0
rcx 0x0
rdx 0x0
rsi 0x0
rdi 0x7fff2e69c670
rsp 0x7fff2e69c670
rbp 0x0
r8 0x0
r9 0x0
r10 0x0
r11 0x0
r12 0x0
r13 0x0
r14 0x0
r15 0x0
> q
```

{% include related_posts.html %}
