---
layout: post
title: "Access Point That Shows You Memes When You Connect To It"
date: 2018-12-21
---

Hello everyone! Today I'm going to show you how I built an access point that is able to send memes to anyone who connects to it. The way it works is simple: First the user connects to the access point from the Wifi settings on their device, and once they try connecting to any website, they'll be shown a meme instead. You can see in the images below how this appears on my phone after it has connected to my wifi.

<img style="width: 60%; height: 60%;" src="../../../assets/img/2018-12-21-wifi-that-only-shows-memes/availablenetworks.jpg" />
<img style="width: 60%; height: 60%;" src="../../../assets/img/2018-12-21-wifi-that-only-shows-memes/meme1.jpg" />
<img style="width: 10%: height: 10%;" src="../../../assets/img/2018-12-21-wifi-that-only-shows-memes/meme2.jpg" />

As you can see, whenever someone tries to connect to a website after connecting to my wireless access point, they will instead be redirected to a web page that shows random memes. Also, if you refresh the page, a different meme will be randomly chosen.

The first thing I did to set this up was grab my new <a href="https://www.raspberrypi.org/">Raspberry Pi</a> that I had just bought and install the <a href="https://www.raspberrypi.org/downloads/raspbian/">Raspbian OS</a> using <a href="https://www.raspberrypi.org/downloads/noobs/">NOOBS</a>. 

<img style="width: 50%; height: 50%; float: right;" src="../../../assets/img/2018-12-21-wifi-that-only-shows-memes/raspberrypi.jpg" />
<img style="width: 50%; height: 50%;" src="../../../assets/img/2018-12-21-wifi-that-only-shows-memes/noobs.jpg" />

Next, I had to configure my Raspberry Pi to work as an access point. This took me a couple of steps. First, I had to go download all of the tools that I would use.

```
sudo apt install hostapd dnsmasq libmicrohttpd-dev
git clone https://github.com/nodogsplash/nodogsplash.git
``` 

I also went online and downloaded a bunch of memes that I liked. Then I went into my /etc/dhcpcd.conf file and added the following line to the file:

```
denyinterfaces wlan0
```

I also had to setup a static IP address for my Raspberry Pi for obvious reasons. I did this by appending the /etc/network/interfaces file with these lines:

```
auto wlan0
iface wlan0 inet static
address 192.168.1.1
netmask 255.255.255.0
```

This allowed me to setup a static IP address of 192.168.1.1. Once I did this, I had to restart dhcpcd using this command:

```
sudo service dhcpcd restart
```

The next step was to configure hostapd. Hostapd was the program that I was going to use to make my Raspberry Pi turn into an access point. I created a file called /etc/hostapd/hostapd.conf and added these settings to it:

```
interface=wlan0 # Use wlan0 interface
driver=nl80211 # Use this driver (you might have to google which driver is the correct one for your wifi card)
ctrl_interface=/var/run/hostapd # Location of hostapd program
ctrl_interface_group=0
ssid=ClickHereForMemes # SSID that will be broadcasted
hw_mode=g
channel=6 # Which channel we are on
wpa=0 # Disable encryption
beacon_int=100 # How often we will send out a beacon
ignore_broadcast_ssid=0 # Don't ignore broadcast requests
```

I had disabled WPA2 encryption because this was a simple project, and I didn't really care whether my Raspberry Pi got hacked or not. But if you wanted to setup WPA2 encryption, you could do it using this configuration file.

I had to tell hostapd where to look for the config file when it starts up on boot, so I opened up /etc/default/hostapd, and replaced this line:

```
#DAEMON_CONF=""
```

with this line:

```
DAEMON_CONF="/etc/hostapd/hostapd.conf"
```

Now that our access point is working, I had to go configure dnsmasq. Dnsmasq is used as a lightweight DNS and DHCP server, which makes it perfect for this project. Since I didn't want to lose the original configuration file, I moved it.

```
sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf.orig
```

I then created a new /etc/dnsmasq.conf file and typed this in there:

```
interace=wlan0 # Which interface we will use
listen-address=192.168.1.1 # What address we are listening on (which should be our static IP)
bind-interfaces # Bind the interfaces
server=8.8.8.8 # Use Google's DNS server
domain-needed
bogus-priv
dhcp-range=192.168.1.50,192.168.1.150,12h # Give IP addresses in the range of 192.168.1.50-150 with a 12 hour lease time
```

Now our access point should be fully setup. Our final step is to setup a captive portal using nodogsplash. This captive portal will work as the website that only shows memes. I had to install nodogslash from the github repository that I cloned earlier.

```
cd ~/nodogsplash
make
sudo make install
```

I went into the /etc/nodogsplash/nodogsplash.conf file and changed the GatewayInterface

```
GatewayInterface wlan0
```

Now we have to actually go and edit the captive portal to display memes instead of displaying a captive portal. The html files for the captive portal are located at /etc/nodogsplash/htdocs/. I edited the splash.html file to look like this:

```
<html>
    <head>
        <title>Random Memes</title>
    </head>
    <body>
        <img />
        <!-- Note: None of these memes were created by me -->
        <script>
            files = ["250freetacos.jpg", "2no3du.jpg", "accidentally.jpeg", "armordoesntmatch.jpeg", "baby.jpg", "backboneofthiscompany.jpg", "becomethevaccumcleaner.jpg", "beforeyougo.jpg", "birdsroom.jpeg", "blueshell.jpg", "bombassdick.jpeg", "bothlinesoftext.jpg", "cameoutofthepurple.jpg", "cleverargument.jpg", "collectingpokemon.jpeg", "computerexpert.png", "coughsyrup.png", "dawgwall.jpg", "deadinside.png", "diarreah.jpg", "formalpictureofbaby.jpg", "freeshipping.jpg", "fuckmatthew.png", "genderpaygap.jpg", "gettingreadyforbed.jpg", "godwhy.png", "goodbyehighschool.jpg", "goodexamples.jpg", "headphones.jpg", "highonacid.jpeg", "hotelblanker.jpg", "howthingsaregoing.jpg", "hulksmash.jpg", "iamconfusion.jpg", "ididntchoosethuglife.jpg", "ihategraveyards.jpeg", "imgonnamissyou.jpg", "improviseadaptovercome.jpg", "infinitelectricity.jpeg", "intellectualdominance.jpeg", "itlookslikeapotato.jpeg", "itswednesday.jpg", "justchillingnaked.jpeg", "lookingatmemesinbathroom.png", "makingdildos.jpeg", "marriage.jpg", "monopolyisbetterthanpizza.png", "mycutepet.jpg", "myheart.jpg", "nodogsallowed.jpg", "nogoodanswer.jpeg", "noitspatrick.jpg", "notacop.jpg", "parallelparking.jpg", "parasites.jpg", "peppapig.png", "pokingpeopleatfunerals.jpg", "pretendingtowork.jpeg", "problemsolving.jpg", "putmebackintoacoma.png", "reallythatsthespermthatwon.jpeg", "recordedconversation.jpg", "researchonisis.jpg", "sayingno.jpg", "schrodingersplates.jpg", "sentencewithouta.jpg", "shoppingcarts.jpg", "shortpeoplesuck.jpg", "sleepinginvideogames.jpeg", "sneezing.png", "solutiontopeopleeatingyourfood.jpeg", "soundmorephotosynthesis.jpg", "studentloans.jpg", "stuffynose.jpg", "stupidshit.jpg", "talktorocks.png", "taskmanager.jpg", "temporarybuildings.jpg", "thankyou.jpg", "thedouchestorecalled.jpeg", "thefemaleisontheright.jpg", "theidiotshouse.jpg", "thesacredtexts.jpg", "timetoretire.jpg", "universityofnorthtexas.jpg", "wheremyloveis.png", "whoistheloveofmylife.png", "whospresentingnext.jpg", "whyarethewaythatyouare.png", "wifeswrath.jpg", "wificonnected.png", "yelpforcoworkers.jpg", "yomamasofatshesataxi.jpg", "youdontfloss.jpg", "yourfat.jpeg", "yourpov.jpg", "yourstupid.jpg"]
            i = Math.floor(Math.random()*files.length);
            var image = document.images[0];
            image.src = "img/" + files[i]
        </script>
    </body>
</html>
```

This is essentially a small script that randomly selects one of the memes I've downloaded and displays it. I also moved all of the images of the memes I downloaded earlier to /etc/nodogsplash/htdocs/img/.

There is just one more step. The computer needs to know that it has to start nodogsplash on boot, so I opened the /etc/rc.local file added "nodogsplash" on top of the line that says "exit 0".

And that's it! All I had to do now was to reboot my computer so that all of my settings would be applied. Thanks for reading this!
