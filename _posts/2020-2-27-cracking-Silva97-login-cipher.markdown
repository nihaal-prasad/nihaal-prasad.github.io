---
layout: post
title: "Cracking Silva97's login-cipher"
date: 2020-2-27
---

Today, I am going to show you how I reverse engineered [this binary](https://crackmes.one/crackme/5db0ef9f33c5d46f00e2c729) and cracked the password.  First, when we run the program, we can see that the program is asking us to input a password. If we input the incorrect password, the program says, "Wrong!" 

<img src="../../../assets/img/2020-2-27-cracking-Silva97-login-cipher/output.png" style="margin-left: auto; margin-right: auto;" />

This is what the main function looks like in Ghidra. Note that I renamed some variables/functions so that the code can be easier to read.

<img src="../../../assets/img/2020-2-27-cracking-Silva97-login-cipher/main.png" style="margin-left: auto; margin-right: auto;" />

There's some interesting stuff happening here. First of all, the print() function will take an encrypted string, decrypt it, and print it out. The encryption appears to be some sort of modified caesar cipher, but I'm not going to go too in depth into this function because it does not matter. The important thing to note is that the line that says 

```uVar1 = check_input(input,"fhz4yhx|~g=5");"```

will check whether the given input is correct or not. If the input is correct, the check_input() function will return 0, else, it will return 1 (this can be figured out by stepping over the function in a debugger using an input value that we know is incorrect). Clearly, our goal here is to figure out what input value makes the check_input() function return 0.

<img src="../../../assets/img/2020-2-27-cracking-Silva97-login-cipher/check_input.png" style="margin-left: auto; margin-right: auto;" />

The second parameter to the check_input() function contains the correct input, but in an encrypted form. The gen_character() function will decrypt the input and return a single character, and successive calls to gen_character() using 0 as the input will return the next decrypted character. The while loop in the middle of the code will check whether each input character is equal to the decrypted character that gen_character() returned. Since we now know where the correct input is being decrypted in the code, all we have to do is set a breakpoint right after the gen_character() function is executed and look at it's return value.

<img src="../../../assets/img/2020-2-27-cracking-Silva97-login-cipher/first_value.png" style="margin-left: auto; margin-right: auto;" />

I used radare2 to step over the first call to the gen_character() function. The decrypted character is 0x63, which is returned in rax. Next, I set rip equal to the second call to the gen_character() function using "dr rip = 0x55f90fc16409", and I stepped over that line of code to figure out what the next decrypted character is. This also ended being 0x63.

<img src="../../../assets/img/2020-2-27-cracking-Silva97-login-cipher/next_value.png" style="margin-left: auto; margin-right: auto;" />

I once again set rip to the gen_character() function and stepped over that line of code to figure out the next decrypted character, and I repeated these steps until I figured out what all of the decrypted characters were, which are shown below:

```
0x63, 0x63, 0x73, 0x2d, 0x70, 0x61, 0x73, 0x73, 0x77, 0x64, 0x34, 0x34, 0x00
```

I assumed 0x00 indicated that this was the end of the string and stopped after seeing it. I sent over these values to the program using python, and I was able to crack the puzzle.

```
python -c "print('\x63\x63\x73\x2d\x70\x61\x73\x73\x77\x64\x34\x34')" | ./login
```

<img src="../../../assets/img/2020-2-27-cracking-Silva97-login-cipher/cracked.png" style="margin-left: auto; margin-right: auto;" />

{% include related_posts.html %}
