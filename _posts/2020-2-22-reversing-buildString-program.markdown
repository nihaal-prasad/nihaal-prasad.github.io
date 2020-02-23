---
layout: post
title: "Reversing the buildString CTF challenge"
date: 2020-2-22
---

Earlier today, I had gone to VMI's 2020 Cyberfusion CTF with my college's CTF team (we won first place!), and I'm going to show how I solved one of their reverse engineering challenges. I was given a binary file called [buildString](../../../assets/other/2020-2-22-reversing-buildString-program/buildString), and upon executing the file, I was greeted with the following output.

<img src="../../../assets/img/2020-2-22-reversing-buildString-program/output.png" style="margin-left: auto; margin-right: auto;" />

I used r2ghidra to decompile the main function, and it is clear that there is a lot going on here.

<img src="../../../assets/img/2020-2-22-reversing-buildString-program/main.png" style="margin-left: auto; margin-right: auto;" />

Essentially, the program is trying to generate a string using a strange algorithm that obfuscates what the string is. It took longer then I want to admit to figure out what I was supposed to do, but I eventually found out that the most important part to reversing this binary lies in the end of the main function, where another function called finalAssembly() is being called.

<img src="../../../assets/img/2020-2-22-reversing-buildString-program/main_end.png" style="margin-left: auto; margin-right: auto;" />

I set a breakpoint at this function and took a look around. There are two function calls at the very beginning that are, presumably, being used to generate the string. I stepped over the two function calls and printed out the value of rdi, which happened to be "ZmxhZ3tFQHp5X1AzM3p5X2xlbW9uc3EzM3p5fQ==".

<img src="../../../assets/img/2020-2-22-reversing-buildString-program/finalAssembly.png" style="margin-left: auto; margin-right: auto;" />

After base64 decoding this value, I obtained the flag.

<img src="../../../assets/img/2020-2-22-reversing-buildString-program/flag.png" style="margin-left: auto; margin-right: auto;" />

{% include related_posts.html %}
