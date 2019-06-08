---
layout: post
title: "Installing Linux In Chromebook Using Crouton"
date: 2019-1-15
---

In a <a href="../14/my-raspberrypi-workstation.html">previous blog post</a>, I mentioned being able to run Linux on a Chromebook. Normally, ChromeOS (which is one of the most useless OSes created) just allows the user to do very basic tasks, such as browsing the internet or accessing documents, and is more geared towards older and more non-technical users. However, it is possible to unlock other potential uses for ChromeOS, one of which is running Linux on the Chromebook. In this post, I'm going to go ahead and show you exactly how I was able to get Ubuntu up and running in my Chromebook using a nice tool called <a href="https://github.com/dnschneid/crouton">crouton</a>.

The first step is to boot the Chromebook in recovery mode. This is done by pressing and holding the escape and refresh keys while pressing the power button at the same time. Once recovery mode opens up, press Ctrl-D to switch to developer mode. It will take a little while for the Chromebook to finish switching to developer mode, but once it is done, you will see a screen that looks like this:

<img src="../../../assets/img/2019-1-15-installing-linux-in-chromebook/boot.jpg">

Wait another minute, and it should reboot into ChromeOS. Sometimes you'll hear two loud beeps coming out of the notebook. Now the Chromebook will always use developer mode whenever you boot into it (unless you turn it off).

The next step is to install crouton. Click <a href="https://goo.gl/fd3zc">here</a> and place the file in the Downloads folder. Next, go into the Chromebook's terminal by pressing Ctrl+Alt+T. Type in "shell" in the black window that pops up to open up the shell. Then run this command to install Linux onto the Chromebook:

```
sudo sh ~/Downloads/crouton -t gnome
```

This will install Ubuntu by default, but you can change which OS is installed. Typing in "sudo sh ~/Downloads/crouton -r list" will show a list of operating systems that are supported by crouton. Typing in "sudo sh ~/Downloads/crouton -r [operating system]" will install that specific OS. To install a specific GUI, use the "-t" switch (and use "-t list" to list out all the different desktop environments).

It will take a few minutes to completely install the OS, and will also ask you to create a username and password for an account on the newly created chroot. Once this is done, you can type in "startgnome" to switch to that operating system (note that this command will change if you've installed a different GUI). 

<img src="../../../assets/img/2019-1-15-installing-linux-in-chromebook/linux.jpg">

You should now be able to use Ctrl+Shift+Alt+Forward and Ctrl+Shift+Alt+Backward (Forwards and backwards keys are found at the upper left corner) to move back and forth between OSes.

{% include related_posts.html %}
