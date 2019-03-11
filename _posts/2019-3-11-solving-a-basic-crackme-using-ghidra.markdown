---
layout: post
title: "Solving a Basic Crackme Using Ghidra"
date: 2019-3-11
---

Recently, the NSA released a free reverse-engineering tool called <a href="https://www.nsa.gov/resources/everyone/ghidra/">Ghidra</a>. Today, I decided to take a look at it and try to solve a basic crackme using this tool. <a href="https://crackmes.one/crackme/5c1a939633c5d41e58e005d1">This</a> is the link to the crackme that I will be trying to solve today.

Once I downloaded the crackme file, I ran it and looked at what the program did. Here is its basic output:
```
enter the magic string
a
wrong string
No flag for you.
```
```
enter the magic string
Some Random Input
too long...sorry no flag for you!!!
```
It looks like it takes some sort of input value, and if that input value is what it expects, then it will print out the flag. If the input value is incorrect or too long, then it will print out a different message. Our goal here is to find out the correct input value.

I started up Ghidra, created a new project, and added the binary file to my project. You can do this by first clicking File->New Project, and then dragging and dropping the binary file into the project.

<img src="../../../assets/img/2019-solving-a-basic-crackme-using-ghidra/ghidra_project.png" />

<img style="float: right" src="../../../assets/img/2019-solving-a-basic-crackme-using-ghidra/symbol_tree.png" />

When I opened the file in Ghidra, a pop-up came up in Ghidra asking me if I wanted to analyze the binary now. I hit yes and waited a second before it was done analyzing the file. Then I went to the symbol tree on the left, typed in "main" to find the main function, and clicked on it. This brought me to the main method, and on the other two windows I could see the main method's assembly and decompiled code.

Here is the decompiled code. I changed a few of the variable names to make it a little bit more readable. You can do this by right-clicking on the variable and clicking "Rename variable."

```
undefined8 main(void)

{
  size_t len;
  size_t len2;
  long lVar1;
  char input [112];
  int counter;
  int strValues;
  
  strValues = 0;
  puts("enter the magic string");
  fgets(input,100,stdin);
  len = strlen(input);
  if (len < 0xc) {
    counter = 0;
    while( true ) {
      len2 = strlen(input);
      if (len2 <= (ulong)(long)counter) break;
      strValues = strValues + (int)input[(long)counter];
      counter = counter + 1;
    }
    if (strValues == 1000) {
      lVar1 = strcat_str();
      printf("flag is flag{");
      counter = 0;
      while (counter < 10) {
        putchar((int)*(char *)(lVar1 + (long)counter));
        counter = counter + 1;
      }
      puts("}");
    }
    else {
      puts("wrong string\nNo flag for you.");
    }
  }
  else {
    puts("too long...sorry no flag for you!!!");
  }
  return 0;
}
```

The first thing that the code does is print out "enter the magic string" and reads in the value that you typed in into a variable that I named "input". It also calculates the length of this input variable and makes sure that it is less than 12 (which makes it a maximum of ten characters plus a newline character). After that, there is a loop that adds up all of the ascii values of each character of the input into an integer that I named "strValues". Then, finally, it prints out the flag if the total values of all the added ascii values equals 1000.

Since I knew that I could type in a maximum of ten characters, the first thing I tried to do was to divide 1000 by 10 to figure out the ascii value of the characters that I needed to type in ten times. Since 1000/10 = 100, and 'd' is 100 in ascii, I decided to type in 'd' ten times and see what happened.

```
$ python -c "print('d' * 10)" | ./rev30
enter the magic string
wrong string
No flag for you.
```

Unforunately, this did not work. At first I didn't understand why this didn't work, but then I realized that I forgot to take into account the fact that the while loop was also adding up the value of the newline character, which had an ascii value of 10. This meant that instead of doing 1000/10 to figure out the ascii value of the character I had to type in 10 times, I had to do (1000-10)/10 to take this into account. This gave me a value of 99, or 'c' in ascii. I tried to use 'c' ten times, and lo and behold:

```
$ python -c "print('c' * 10)" | ./rev03 
enter the magic string
flag is flag{!#&*/5<DMW}
```

Thanks for reading this writeup. I hope you enjoyed it.
