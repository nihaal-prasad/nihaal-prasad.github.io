---
layout: post
title: "Hooking Linux Function Calls Via LD_PRELOAD"
date: 2021-1-7
---

Hooking functions is the act of replacing a pointer to a function with a different pointer so that another function is called instead of the original function. For example, suppose there was a function `x()` being called inside of a program. If an attacker has write permissions to the binary file, then the attacker can replace the code calling `x()` with code that calls some malicious function `y()` instead.

Below contains the code that will be used for this tutorial. The code contains a `check_pass()` function, which checks whether a user-supplied password is equal to the string "p@ssw0rd". In real life, the program would (hopefully) not use a hardcoded plaintext password and instead query a database of hashed passwords. For this example, let's ignore the fact that the password could be retrieved with the `strings` command, and let's assume that the attacker has no idea what the password is.
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 100

void correct() {
    printf("You typed in the correct password!\n");
}

void incorrect() {
    printf("You typed in an incorrect password!\n");
}

void check_pass() {
    char buf[BUF_SIZE];
    printf("Enter the password: ");
    fgets(buf, BUF_SIZE, stdin);
    if(strncmp("p@ssw0rd\n", buf, 10) == 0) {
        correct();
    } else {
        incorrect();
    }
}

void main() {
    check_pass();
}
```

Compiling and executing the program displays the following:
```sh
$ gcc -o check_pass check_pass.c
$ ./check_pass
Enter the password: Hello
You typed in an incorrect password!
$ ./check_pass
Enter the password: p@ssw0rd
You typed in the correct password!
```

Let's say that the attacker wants to execute the `correct()` function without even knowing the correct password. The attacker will first want to find the address of the code that calls `incorrect()` inside of the binary file. Then, the assembly instruction can be patched so that it calls `correct()` instead of `incorrect()`.
```sh
$ objdump -dj .text -M intel check_pass| grep correct
0000000000001155 <correct>:
0000000000001168 <incorrect>:
    11d8:       e8 78 ff ff ff          call   1155 <correct>
    11e4:       e8 7f ff ff ff          call   1168 <incorrect>
```

From this object dump, we can see that the line of assembly code that calls `incorrect()` is `0x11e4` bytes into the file. The function `correct()` is located `0x1155` bytes into the file. This means that if we change the instruction that is `0x11e4` bytes into the file to call `0x1155`, we can execute the `correct()` function without typing in the correct password.

I wrote a small Python script for patching binaries in one shell command, which can be downloaded <a href="https://github.com/nihaal-prasad/Patcher/tree/main">here</a>. We will use this script to modify the instruction `0x11e4`bytes into the file. This way, the program will still call `correct()` even if an incorrect password is given.
```sh
$ ./patcher.py -a check_pass 0x11e4 CALL 0x1155
$ objdump -dj .text -M intel check_pass| grep correct
0000000000001155 <correct>:
0000000000001168 <incorrect>:
    11d8:       e8 78 ff ff ff          call   1155 <correct>
    11e4:       e8 6c ff ff ff          call   1155 <correct>

$ ./check_pass
Enter the password: WrongPassword
You typed in the correct password!
```

That's all it takes to overwrite a function pointer. Now, let's suppose that the attacker isn't satisfied with having a correct password; the attacker wants to have a working shell be executed as well. In order to do this, the attacker needs to create his own function and replace the instruction at `0x11e4` with a pointer to his own function. There are several methods of doing this, but the method that will be discussed today involves using the `LD_PRELOAD` trick.

Typically, whenever a process knows that it needs to use a function from an external library, the process needs to load that library into memory and resolve symbols at runtime. There is an environment variable called `LD_PRELOAD`, which will contain a list of libraries that will be loaded before any other library. If the attacker modifies the `LD_PRELOAD` variable, the attacker can specify his own library to load.

In order to insert a malicious function into the program, the attacker will do the following:
1. Find a legitimate library function (let's suppose this function is called `x()`). 
2. Patch a `CALL` instruction to call the PLT entry of the legitimate library function. (In our case, this would be the `CALL` instruction at `0x11e4`.)
3. Create a malicious library that contains a function with the same name as the legitimate library function.
4. Use `LD_PRELOAD` to load the malicious library.

Because `LD_PRELOAD` loads the malicious library before the legitimate library, the symbol for `x()` will be resolved to be the function inside of the malicious library instead of the function inside of the legitimate library. Because the attacker has patched some `CALL` instruction with the address of the PLT for `x()`, whenever that call instruction is executed, the attacker's version of `x()` will be called.

For step 1, let's first find some of the PLT entries that are being used in the program.
```sh
$ objdump -dj .text -M intel check_pass| grep plt
    112e:       e8 2d ff ff ff          call   1060 <__cxa_finalize@plt>
    1160:       e8 cb fe ff ff          call   1030 <puts@plt>
    1173:       e8 b8 fe ff ff          call   1030 <puts@plt>
    118f:       e8 ac fe ff ff          call   1040 <printf@plt>
    11a7:       e8 a4 fe ff ff          call   1050 <fgets@plt>
```

Here, we can see that the following functions are called at some point in the program:
* `__cxa_finalize(void *d)`
* `puts()`
* `printf()`
* `fgets()`

Any of these functions can be overwritten, but for this example, we'll choose `__cxa_finalize(void *d)`, which is typically called at the end of a program. The PLT entry for this function is `0x1060`. Now, we can use Python to patch the instruction at `0x11e4` to call `__cxa_finalize(void *d)`. To complete step 2, we'll use my <a href="https://github.com/nihaal-prasad/Patcher">Patcher</a> script again.

```sh
$ ./patcher.py -a check_pass 0x11e4 call 0x1060
$ objdump -dj .text -M intel check_pass | grep 11e4
    11e4:       e8 77 fe ff ff          call   1060 <__cxa_finalize@plt>
```

For step 3, we need to create a malicious library file that redefines the `__cxa_finalize(void *d)` function. Note that this function takes in one `void *` as a parameter, so we need to include that `void *` in the function declaration.
```sh
$ cat library.c
#include <stdio.h>
#include <stdlib.h>

void __cxa_finalize(void *d) {
    system("/bin/sh");
}

$ gcc -shared -fPIC -o library.so library.c
```

Finally, to get ourselves a shell, run the command shown below. This will set the `LD_PRELOAD` environment variable to be our malicious environment variable so that our version of the `__cxa_finalize(void *d)` function gets called. Note that the second shell prompt ($) is caused by our version of `__cxa_finalize(void *d)` being executed.
```sh
$ LD_PRELOAD=$PWD/library.so ./check_pass
Enter the password: SomethingWrong
$ whoami
nihaal
```

As you can see write here, an attacker can use this technique to hook their own function and execute arbitrary code. In real life, people can use this technique to modify the behavior of a program and bypass security features (for example, one could hook a function checking a license key with their own function that does not check the license key).

References:
1. https://binaryresearch.github.io/2019/08/29/A-Technique-for-Hooking-Internal-Functions-of-Dynamically-Linked-ELF-Binaries.html
2. https://www.baeldung.com/linux/ld_preload-trick-what-is
3. https://www.keystone-engine.org/
4. https://www.malwaretech.com/2015/01/inline-hooking-for-programmers-part-1.html

{% include related_posts.html %}
