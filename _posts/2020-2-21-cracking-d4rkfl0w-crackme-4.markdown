---
layout: post
title: "Cracking D4RKFL0W's Crackme-4"
date: 2020-2-21
---

Today, I will be going over how I cracked [this crackme](https://crackmes.one/crackme/5e0fa43b33c5d419aa01351e). When I first executed the program, it started off by asking me to enter some password, so it is clear that our goal here is to figure out what this password is.

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/execution.png" style="margin-left: auto; margin-right: auto;" />

I opened up this binary in radare2 and saw that the main function was calling another function called prompt().

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/main.png" style="margin-left: auto; margin-right: auto;" />

I stepped into this function and saw that this function would print out "Please enter the password:" and read a line of input. There was another interesting function called Vault::checkPassword() being called near the end of the function.

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/prompt.png" style="margin-left: auto; margin-right: auto;" />

I stepped into Vault::checkPassword(). It seems as though this function calls another function called PasswordGen::buildPassword(), which probably builds the password, and then calls PasswordGen::checkPassword(), which is the function that we care about the most. If the password looks good, then it will print out a message stating that it is the correct password.

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/checkPasswordVault.png" style="margin-left: auto; margin-right: auto;" />

Inside of Vault::checkPassword(), there are function calls to c3(), c0(), c1(), c2(), c4(), c8(), c5(), c6(), and c7(). Each of these function calls generate and return one of the characters for the password, and the correct index of each character is indicated by the number coming after c (i.e., c3() returns the 3rd character of the password, c0() returns the 0th character of the password, etc.). After each function call, the corresponding character for the user's input is checked against the character that was generated, and if the input does not equal the character, the program exits.

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/checkPasswordGen.png" style="margin-left: auto; margin-right: auto;" />

I set a breakpoint right after c3() executes, and I recorded the value of rax, which was 3a.

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/c3.png" style="margin-left: auto; margin-right: auto;" />

I repeated this by setting breakpoints right after the other function calls to c0(), c1(), c2(), c4(), c8(), c5(), c6(), and c7(), and I continued to record the values of rax. These were the results:

```
c3(): rax = 0x3a
c0(): rax = 0x55
c1(): rax = 0x36
c2(): rax = 0x2d
c4(): rax = 0x59
c8(): rax = 0x2b
c5(): rax = 0x4c
c6(): rax = 0x2e
c7(): rax = 0x22
```

I had recreated all of the correct values for the password. Now all I had to do was to reorder the password values so that the 0th value comes first and the 8th value comes last and send the input using the below command.

```
python -c "print('\x55\x36\x2d\x3a\x59\x4c\x2e\x22\x2b')" | ./Crackme-4
```

<img src="../../../assets/img/2020-2-21-cracking-d4rkfl0w-crackme-4/correct.png" style="margin-left: auto; margin-right: auto;" />

{% include related_posts.html %}
