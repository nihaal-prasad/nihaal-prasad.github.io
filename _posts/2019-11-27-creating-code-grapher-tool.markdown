---
layout: post
title: "Creating a Code Grapher Tool"
date: 2019-11-27
---

A little while ago, I was working on some reverse-engineering CTF problems. I was trying to figure out what a particular function was doing by giving it a particular input, stepping over the function, reading its output, and then repeating those three steps over and over again with different input values so that I could try to guess what the function was trying to do. I then realized that I could easily automate that process using a script, which is where the idea for <a href="https://github.com/nihaal-prasad/Code-Grapher">this simple tool</a> came from.

To give you an example of how this works, suppose you wanted to reverse engineer the following function:
```
0000000000001135 <magic>:
    1135:	55                   	push   %rbp
    1136:	48 89 e5             	mov    %rsp,%rbp
    1139:	89 7d fc             	mov    %edi,-0x4(%rbp)
    113c:	8b 45 fc             	mov    -0x4(%rbp),%eax
    113f:	0f af 45 fc          	imul   -0x4(%rbp),%eax
    1143:	5d                   	pop    %rbp
    1144:	c3                   	retq   

0000000000001145 <main>:
    1145:	55                   	push   %rbp
    1146:	48 89 e5             	mov    %rsp,%rbp
    1149:	48 83 ec 10          	sub    $0x10,%rsp
    114d:	bf 03 00 00 00       	mov    $0x3,%edi
    1152:	e8 de ff ff ff       	callq  1135 <magic>
    1157:	89 45 fc             	mov    %eax,-0x4(%rbp)
    115a:	8b 45 fc             	mov    -0x4(%rbp),%eax
    115d:	89 c6                	mov    %eax,%esi
    115f:	48 8d 3d 9e 0e 00 00 	lea    0xe9e(%rip),%rdi        # 2004 <_IO_stdin_used+0x4>
    1166:	b8 00 00 00 00       	mov    $0x0,%eax
    116b:	e8 c0 fe ff ff       	callq  1030 <printf@plt> 1170:	b8 00 00 00 00       	mov    $0x0,%eax
    1175:	c9                   	leaveq 
    1176:	c3                   	retq   
    1177:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
    117e:	00 00
```
If you use my script and run the following command, a graph will appear, which will display all of the outputs for the magic() function when you use inputs in the range [0,100] (0 is inclusive, 100 is exclusive).

<img style="float: center;" src="../../../assets/img/2019-11-27-creating-a-code-grapher-tool/graph.png" />

Furthermore, there is also a list of points printed out in the output:
```
Points:
[(0, 0), (1, 1), (2, 4), (4, 16), (3, 9), (5, 25), (7, 49), (6, 36), (8, 64), (9, 81), (14, 196), (10, 100), (11, 121), (12, 144), (13, 169), (17, 289), (16, 256), (15, 225), (18, 324), (19, 361), (23, 529), (20, 400), (21, 441), (22, 484), (24, 576), (26, 676), (25, 625), (28, 784), (27, 729), (30, 900), (29, 841), (32, 1024), (31, 961), (33, 1089), (35, 1225), (36, 1296), (34, 1156), (37, 1369), (38, 1444), (41, 1681), (39, 1521), (40, 1600), (42, 1764), (43, 1849), (46, 2116), (44, 1936), (45, 2025), (47, 2209), (49, 2401), (50, 2500), (48, 2304), (51, 2601), (53, 2809), (54, 2916), (52, 2704), (55, 3025), (56, 3136), (57, 3249), (58, 3364), (60, 3600), (59, 3481), (61, 3721), (62, 3844), (64, 4096), (63, 3969), (65, 4225), (66, 4356), (71, 5041), (67, 4489), (68, 4624), (70, 4900), (69, 4761), (72, 5184), (76, 5776), (74, 5476), (75, 5625), (73, 5329), (80, 6400), (81, 6561), (78, 6084), (77, 5929), (79, 6241), (85, 7225), (83, 6889), (84, 7056), (86, 7396), (82, 6724), (88, 7744), (89, 7921), (87, 7569), (90, 8100), (91, 8281), (94, 8836), (93, 8649), (92, 8464), (96, 9216), (95, 9025), (97, 9409), (98, 9604), (99, 9801)]
```
Essentially, the way that this works is that my Python script will set two breakpoints. In this example, the breakpoints were set at sym.magic and sym.main+21. At sym.magic, the input register (which is set to be rdi) is changed to be one of the input values in the specified range. At sym.main+21, eax is read as the output value, and the point (rdi, eax) is plotted onto the graph.

The script is implemented using <a href="https://github.com/radareorg/radare2-r2pipe">r2pipe</a>, which allows me to use radare2 commands from within Python and receive their output, and <a href="https://matplotlib.org/">matplotlib</a>, which allows me to easily create graphs in a few lines of code.

There are several options built within the script that you can use to aid your analysis. You can see all of the options below:
```
$ python grapher.py -h 
usage: grapher.py [options] filename start stop input output range

Analyzes specified lines of code by executing the code using the given input
values, recording the output, and displaying the input and output in a graph.

positional arguments:
  filename              The name of the executable you would like to analyze.
  start                 The first breakpoint will be set at this location. At
                        the breakpoint, the input register or memory location
                        will be changed to the next value in the range.
  stop                  The second breakpoint will be set at this location. At
                        the breakpoint, the output will be recorded.
  input                 The register or memory location that contains the
                        input value that should be bruteforced. Will be
                        displayed on the x-axis. Example: "eax". If using a
                        memory location, please specify the location using
                        m[location]. Example: "m[rbp-0x8]".
  output                The register or memory location that contains the
                        output values that should be checked after the code is
                        executed. Will be displayed on the y-axis. Example:
                        "eax". If using a memory location, please specify the
                        location using m[location]. Example: "m[rbp-0x8]".
  range                 The range of values that should be used for the input
                        during the bruteforce process. Should be in the form
                        "[lower,upper]" or "[lower,upper,step]". For example:
                        [0,101,5] will use 0, 5, 10, ..., 95, 100 as the x
                        values in the graph. These must be in base 10
                        (hexadecimal or binary will not work).

optional arguments:
  -h, --help            show this help message and exit
  -t [THREADS], --threads [THREADS]
                        The number of threads that will be used during
                        execution. Default value is 5.
  -in [INPUT_FILE], --standard-input [INPUT_FILE]
                        Uses the 'dor stdin=[INPUT_FILE]' command in radare2
                        to make the executable read standard input from a
                        given file instead of having the user type it in.
  -il [INPUT_LENGTH], --input-length [INPUT_LENGTH]
                        The amount of bytes placed at the input memory
                        location. Default value is 1, but this will be
                        automatically adjusted if it is too small. Is only
                        used if the input is a memory location and not a
                        register.
  -ol [OUTPUT_LENGTH], --output-length [OUTPUT_LENGTH]
                        The amount of bytes read at the output memory
                        location. Must be equal to either 1, 2, 4, or 8.
                        Default value is 1. Is only used if the output is a
                        memory location and not a register.
  -e [COMMANDS], --execute [COMMANDS]
                        Executes the given r2 commands in radare2 right after
                        the debugger hits the first breakpoint, but before the
                        input value is set. Example: -e "dr ebx = 7" will
                        always set ebx equal to 7 at the first breakpoint.
                        Multiple commands can be separated by a semicolon.
  -hx, --x-axis-hex     Displays the x-axis in hexadecimal instead of denary.
  -hy, --y-axis-hex     Displays the y-axis in hexadecimal instead of denary.
  -j, --jump            Instead of running all of the code that comes before
                        the breakpoint, if this option is set, rip/eip will
                        immidiately be set to the start value as soon as the
                        program opens. This will essentially jump over any
                        code that comes before the first breakpoint, and it
                        will make the program only execute the code between
                        the starting and stopping breakpoints.
```

The script can be found <a href="https://github.com/nihaal-prasad/Code-Grapher">here</a>.

    
