---
layout: post
title: "My Raspberry Pi Workstation Setup With VNC, hostapd, and dnsmasq"
date: 2019-1-14
---

I had three raspberry pis lying around that were all connected to an ethernet switch, and I wanted to setup software on all three of them to play around with. However, I only had one keyboard, one mouse, and one monitor that I could use, which meant that every time I switched which raspberry pi I wanted to use, I would have to reconnect the wires for the keyboard, mouse, and monitor to the raspberry pi I wanted to use. This setup was quite messy, so I wanted a way for me to easily access all three of my raspberry pis while I'm doing projects on them. My first thought was to invest in a <a href="https://en.wikipedia.org/wiki/KVM_switch">KVM switch</a> that supported HDMI (since my raspberry pis only had an HDMI port for a monitor), but the KVM switches that suited my needs seemed to be way out of my budget, which is why I decided to find a cheaper way to solve this problem.

<img src="../../../assets/img/2019-1-14-my-raspberrypi-workstation/setup.jpg">

I also happened to have a Chromebook lying around, and I realized that maybe there was a way for my Chromebook to take remote control over the three raspberry pis over VNC. So the first thing I did was enable VNC on all of my raspberry pis. This was the easiest step for me to setup this network because VNC is built into the raspbian operating system, so all I had to do was to type in this command:

```
sudo raspi-config
```

And then go into the interfacing options and turn on VNC.

<img src="../../../assets/img/2019-1-14-my-raspberrypi-workstation/raspiconfigvnc.png">

I also <a href="../15/installing-linux-in-chromebook.html">installed an Ubuntu chroot</a> on my Chromebook and installed a VNC client on that Ubuntu chroot.

But I ran into another problem at this point. In order for VNC to work, the Chromebook has to be on the same network as the computer it is trying to connect to. How could I allow my Chromebook to be on the same private ethernet network as my raspberry pis? My Chromebook didn't even have an ethernet port, so how could it connect to my ethernet switch? I briefly considered buying a <a href="https://www.amazon.com/s/ref=nb_sb_ss_c_1_15?url=search-alias%3Delectronics&field-keywords=usb+to+ethernet+adapter&sprefix=usb+to+ethernet%2Celectronics%2C181&crid=BMH55CBHB81E">USB to ethernet adapter</a> so that my Chromebook could access the network, but then I found a free solution (that took a little bit more work on my end). What if I turned one of my raspberry pis into a wireless access point that bridged to my ethernet network so that my Chromebook could connect to it?

So I went ahead and installed all of the tools that I needed to do this.

```
sudo apt install hostapd dnsmasq bridge-utils
```

I went into /etc/dhcpcd.conf to give my raspberry pi a static IP address, which I needed to have for obvious reasons. I also denied wlan0 from being used as an interface. I used this command to restart dhcpcd afterwards.

```
sudo service dhcpcd restart
```

My next step was to configure hostapd, the main tool used to turn my raspberry pi into an access point. I created a file called /etc/hostapd/hostapd.conf and added some details about the name of the interface, the SSID, the bridge interface I was planning on using, and the WPA2 key password to the file. Then I edited /etc/default/hostapd to add the location of my configuration file.

Now I had to configure dnsmasq, which was a tool used as both a DHCP server and a DNS server.

My final step was to be able to bridge between the wlan0 interface and the eth0 interface. This just took two commands.

```
brctl addbr br0
brctl addif br0 eth0
```

Then I restarted my raspberry pi, and everything worked just fine.

<img src="../../../assets/img/2019-1-14-my-raspberrypi-workstation/network.jpg">

(Image is not to scale (obviously). Also, please excuse my horrible Microsoft paint skills. I was not born with any artistic capabilities.)

So, to recap, basically all of my Raspberry Pis are connected to a network switch via ethernet, which is represented with the green lines. My first Raspberry Pi is setup to be a wireless access point that extends this network, and this is done to allow my chromebook, which has an Ubuntu <a href="https://github.com/dnschneid/crouton#whats-a-chroot">chroot</a> on it, to access the network via wifi, which is represented by the four poorly drawn blue curves near the chromebook. Unfortunately, I only had one keyboard, one monitor, and one mouse, which were represented by the purple I/O lines in the diagram. This was a little bit annoying for me because if I wanted to switch which Raspberry Pi is currently being used, I would have to reconnect all of the wires for the keyboard, monitor, and mouse to that Raspberry Pi. Since <a href="https://en.wikipedia.org/wiki/KVM_switch">KVM switches</a> are way too expensive for me to buy (and a little unnecessary), I setup VNC servers on all three of them. My chromebook, which has a VNC client installed in it, is able to access all three Raspberry Pis through the VNC service.
