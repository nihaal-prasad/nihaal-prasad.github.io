---
layout: post
title: "How I Made A Small Home Backup Server Using Rsync Over SSH"
date: 2018-11-25
---

Have you ever been worried about losing your files? I definitely am. A little while back, I was working on a coding project for one of my computer science classes at college, when I realized that if something happened to my laptop, such as getting hit with a virus infection or being dropped, I could easily lose all of my hard work. Imagine how horrible it would be if you lost/damaged your school computer and didn't have any backups!

<img style="float: right;" src="../../../assets/img/2018-11-24-rsync-backup-server/Crying_GIF.gif" />

Since I didn't want to ever have to deal with redoing an assignment, I decided that I would figure out how to make a simple backup server at home so that I could backup all of my files when I'm outside the house. After some research, I found a program called <a href="https://en.wikipedia.org/wiki/Rsync">rsync</a> that could easily sync your files with another computer over SSH. I'm now going to go over what I did to set this up.

The only thing I needed (aside from the laptop that I was going to backup) was an SSH server. Since I only required enough memory to backup the things that I was doing in college and some other random files that I had on my computer, I didn't need to buy some expensive server hardware. I had an old cheap <a href="https://www.cnet.com/products/dell-optiplex-380/specs/">Dell Optiplex 380 Desktop</a> sitting in my basement, which only had 2GB of RAM, a 256GB hard drive, and Ubuntu OS installed on it, and it worked just fine for this project (because they didn't want it, one of my teachers from high school gave it to me as a prize for winning something, and it was just sitting in my basement since). Although I haven't tried it, someone could also probably do the same thing by setting up the backup server in a <a href="https://www.raspberrypi.org/">Raspberry Pi</a> connected to an external hard drive.

<img style="margin-left: auto; margin-right: auto;" src="../../../assets/img/2018-11-24-rsync-backup-server/Dell Optiplex.jpg" />

Anyways, the first thing that I did was setup an <a href="https://en.wikipedia.org/wiki/Secure_Shell">SSH server</a> on my desktop computer. Since I was going to be using rsync on top of SSH, it was important that I was able to get this SSH server up and running. I used <a href="https://en.wikipedia.org/wiki/OpenSSH">openSSH</a> because it's free and open source. To install openssh, I used this command on my Desktop computer:
```
$ sudo apt-get install openssh-server
```
Once I installed the SSH server, it started running by default. I checked that SSH is working by SSHing into my computer.
```
$ ssh admin@localhost
admin's password:
>
```
From there, I decided that I wanted to create a new user just for doing backups for security purposes.
```
$ useradd backupuser
$ passwd backupuser
Changing password for user backupuser.
New UNIX password:
Retype new UNIX password:
passwd: all authentication tokens updated successfully.
```
The next thing I wanted to do was to generate SSH keys on my laptop (which is NOT the same computer as the one that has the SSH server. This is the computer that I use for my school assignments).
```
$ ssh-keygen -t rsa
Generating public/private rsa key pair.
Enter file in which to save the key (/home/user/.ssh/id_rsa): /home/user/.backups/id_rsa
Created directory '/home/user/.backups/'.
Enter passphrase (empty for no passphrase): 
Enter same passphrase again: 
Your identification has been saved in /home/user/.backups/id_rsa.
Your public key has been saved in /home/user/.backups/id_rsa.pub.
```
I then copied the keys from my laptop onto where the SSH server was:
```
$ ssh-copy-id -i /home/user/.backups/id_rsa.pub backupuser@[internal ip address]
```
Now that the public key was copied, I was able to use passwordless SSH authentication to login to my SSH server. Next, I opened the server's SSH config file (located at /etc/ssh/sshd_config) using vim to change some settings for the SSH server. I made sure that I denied the admin user from using SSH, which made sure that the backup user was the only user that could use SSH.
```
DenyUsers admin
```
I changed the banner to be the text inside the /etc/issue.net file. I also changed the /etc/issue.net file to say "ABSOLUTELY NO TRESPASSING" to anyone who tried to connect to the server.
```
Banner /etc/issue.net
```
Then I disabled X11 forwarding.
```
X11Forwarding no
```
I disabled password authentication (it was no longer necessary after I copied the keys earlier).
```
PasswordAuthentication no
```
I disabled root login because that could become a security risk.
```
PermitRootLogin no
```
I also changed the port number so that it would be harder to find (most port scanners will not scan for this port).
```
Port 2222
```
Finally, after the SSH server was completely setup, I installed the rsync program on my laptop.
```
$ sudo apt-get install rsync
```
And then I wrote a simple script that would use the rsync program (which is displayed below). I saved the script as "backup.sh" and ran it as root.
```
sudo rsync -avz -e 'ssh -i /home/user/.backups/id_rsa -p 2222' --progress /home/user/Desktop/ backupuser@[internal ip address]:~/Desktop/
sudo rsync -avz -e 'ssh -i /home/user/.backups/id_rsa -p 2222' --progress /home/user/Documents/ backupuser@[internal ip address]:~/Documents/
sudo rsync -avz -e 'ssh -i /home/user/.backups/id_rsa -p 2222' --progress /home/user/Pictures/ backupuser@[internal ip address]:~/Pictures/
```
It successfully copied all of the files in my Desktop, Documents, and Pictures folders over to the SSH server's folders. Our backup server has worked!

There was still a tiny bit more work to do. At the moment, the server would only work if my laptop was on the same network as my server. However, if I moved my laptop to a separate network (such as my college's WiFi network), it would not work properly. To fix this, I logged onto my router settings and port-forwarded port 2222, which is where my SSH server is listening. I then changed my backup.sh script to use my public ip address instead of my internal ip address, and tried again. It worked!

<img style="margin-left: auto; margin-right: auto;" src="../../../assets/img/2018-11-24-rsync-backup-server/port-forwarding.png" />

There was another problem. My router currently has a dynamic IP address, meaning that its IP address could change at any moment. As a result, I would have to change the IP address in my script if it changed, which could become a little bit annoying. So I went ahead and created an account at <a href="https://www.noip.com/">No-IP</a>, which is a service that grants you up to five free domain names for someone that has a dynamic IP address (you can buy more domain names if you want, but you get five of them for free). 

<img style="margin-left: auto; margin-right: auto;" src="../../../assets/img/2018-11-24-rsync-backup-server/no-ip.png" />

It was pretty easy to navigate No-IP's web interface (I promise this wasn't sponsored). I just used the "quick add" option on the dashboard, setup my hostname and domain name (since I only had a free account, I had to use one of their premade domain names). Then I had to go to my router's settings and tell my router where the Dynamic Domain Name Server was located so that every time my dynamic IP address changed, my router would alert No-IP to change which IP address the domain name is pointing to.

<img style="margin-left: auto; margin-right: auto;" src="../../../assets/img/2018-11-24-rsync-backup-server/dynamic dns.png" />

After setting up my domain in No-IP's website, I was able to use it to connect to my backup server anywhere in the world by using the domain name. 

There was just one more little thing I had to do. By default, only root is able to send a shutdown signal to the computer over SSH. In contrast, I wanted my backup user to also be able to use the poweroff command so that I could remotely power down my server after I was done with my backups. I went ahead and <a href="https://askubuntu.com/questions/1190/how-can-i-make-shutdown-not-require-admin-password/1399#1399">changed the policies in the /usr/share/polkit-1/actions/org.freedesktop.consolekit.policy file</a>, which gave my backup user permission to use the poweroff command. I also added one extra line in my backup.sh script so that I would automatically shut down the server once I was done with it, making my backup.sh script look like this:

```
sudo rsync -avz -e 'ssh -i /home/user/.backups/id_rsa -p 2222' --progress /home/user/Desktop/ backupuser@[domain name I got from No-IP's website]:~/Desktop/
sudo rsync -avz -e 'ssh -i /home/user/.backups/id_rsa -p 2222' --progress /home/user/Documents/ backupuser@[domain name I got from No-IP's website]:~/Documents/
sudo rsync -avz -e 'ssh -i /home/user/.backups/id_rsa -p 2222' --progress /home/user/Pictures/ backupuser@[domain name I got from No-IP's website]:~/Pictures/
ssh -i /home/user/.backups/id_rsa -p 2222 backupuser@[domain name I got from No-IP's website] "systemctl poweroff -i"
```

Finally, everything was completed. The backup server works quite well so far, and I am quite happy with its output.

{% include related_posts.html %}
