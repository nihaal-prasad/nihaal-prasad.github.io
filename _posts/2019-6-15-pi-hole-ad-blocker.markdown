---
layout: post
title: "Network-wide Ad Blocking Using Pi-hole"
date: 2019-6-15
---

As someone who loves using Raspberry Pis, I thought that it would be a fun project to setup an ad blocking DNS server on my network using <a href="https://pi-hole.net/">Pi-hole</a>. Pi-hole, for those who do not know, is a DNS server that automatically tries to detect and block domains that have known advertisements. The way that it works is that normally, if your computer encounters a domain name without knowing its IP address, it will send a query to a DNS server, which will tell the computer what the corresponding IP address is (or forward it to another DNS server if it does not know). Pi-hole uses blacklists of domains to make sure that if you send a DNS request to it, it prevent you from accessing those domains.

So lets get right into how I did this. The first thing I did was that I SSH'ed into my raspberry pi and typed in the below command to start installing Pi-hole.

```
curl -sSL https://install.pi-hole.net | bash
```

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/installing.png">

After that, I had to select the upstream DNS provider, which is who my raspberry pi would forward DNS requests to if the DNS query is not on the blacklist. I like to use <a href="https://www.quad9.net/">Quad9's DNS server</a> as opposed to Google's DNS server (or any other popular DNS server) because Quad9 uses threat intelligence in order to block malware and other threats.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/quad9.png">

Pi-hole also asked me if I wanted to change the third-party lists that they use or if I wanted to change which protocol they used. I kept the default values.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/lists.png">

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/protocols.png">

I kept my current network settings.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/ip.png">

I kept the default values for everything else.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/webadmin.png">

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/lighttpd.png">

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/logging.png">

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/privacy.png">

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/complete.png">

Once everything was done on my raspberry pi's side, I had to go into my router settings to change the DNS server settings so that they pointed to my new DNS server. I used Quad9's DNS server as my backup secondary DNS server in case my raspberry pi failed for whatever reason.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/dnssettings.png">

I also went ahead and changed my current raspberry pi's IP address to be a static IP address that the DHCP server could not change.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/staticip.png">

Finally, everything was finished. I could now login to my Pi-hole's web admin page and see it all running.

<img src="../../../assets/img/2019-6-15-pi-hole-ad-blocker/webpage.png">

{% include related_posts.html %}
