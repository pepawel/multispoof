### Multispoof: Parallel spoofing for throughput increase

#### Notice

Because of changes in Linux kernel, particularly in netfilter/iptables, multispoof won't run on recent systems (see notes in requirements section).

It can be fixed, but probably won't be, as I don't have enough time. I'll welcome patches though.

#### What is multispoof?

Multispoof is an application, which exploits weak, address based authentication very frequently implemented by ISPs in Ethernet networks. In such networks customers are identified with IP-MAC address pairs, and only those paying ISP are granted access to the Internet.

Multispoof uses IP and MAC spoofing to impersonate legitimate customers. The idea is not new, but multispoof does it in a smart way. As it impersonates only inactive customers, there is no address conflicts. And using multiple addresses in parallel in combination with load balancing allows to achieve much higher transfer rates.

It could be compared with download accelerating software, because higher throughput is achieved with multiple transmissions. The difference is that multispoof operates on layers 2 and 3 of the OSI model. In contrast, download accelerator uses multiple TCP streams – the fourth layer of OSI model.

I've created multispoof as a software project for my M.Sc. thesis, so entire application (version 0.6.1) is documented quite precisely in there, unfortunately only in Polish language. I've included the pdf of my thesis in the `mscthesis-pl.pdf` file. I've spent entire chapter on spoofing detection and prevention techniques, so if you are an ISP, you may be interested too.

#### Legal

Multispoof was created to demonstrate the risk of using weak authentication methods, and is meant to be used for testing purposes. It is only a tool and you are responsible for its usage, especially for abuse of your ISP network.

#### Features

* Accelerates throughput multiple times using parallel spoofing with load balancing.
* When aggresively used can fill up your ISP Internet link, so its great for testing maximal throughput of provider :)
* IP and MAC spoofing.
* Only inactive addresses are used for spoofing. No address conflicts.
* Detection of active hosts performed with ARP scanning.
* Only addresses permitted to access external network (usually Internet) are used in spoofing process. Connectivity testing is easily configurable.

#### Requirements

In order to run multispoof you need:

* [Linux](http://www.kernel.org/) kernel with following features enabled:
  * Tun/tap wirtual network interfaces.
  * Unix sockets.
  * Netfilter with SNAT and DNAT targets, nth and owner matches (nth is in [patch-o-matic](http://www.netfilter.org/patch-o-matic/) package).
    * **Note 1:** Part of the owner match functionality was [removed](http://www.kernel.org/git/?p=linux/kernel/git/torvalds/linux-2.6.git;a=commit;h=34b4a4a624bafe089107966a6c56d2a1aca026d4). Unfortunately, multispoof requires that part (particularly --sid-owner option) for correct operation. For now, the only workaround is to use older kernel version - 2.6.11 works ok.
    * **Note 2:** As nth match was removed from [patch-o-matic](http://www.netfilter.org/patch-o-matic/) it is not possible to run multispoof using recent version of PoM. Possible workarounds: use older PoM or rewrite multispoof load-balancing code to make use of statistic match.
* [Iptables](http://www.netfilter.org/projects/iptables/index.html) userspace utility with targets and matches mentioned above.
* Ip tool from [iproute2](http://developer.osdl.org/dev/iproute2/) suite.
* [Bash](http://www.gnu.org/software/bash/bash.html) (only version 3.0 tested).
* [Libpcap](http://www.tcpdump.org/) 0.9.3 or higher.
* [GLib](http://www.gtk.org/) 2.6 or higher.
* [Nmap](http://www.insecure.org/nmap/) (only 3.81 version tested, but other versions should also work)
* Standard unix text tools.

Compilation process requires also:

* C compiler ([gcc](http://gcc.gnu.org/) 3.3.5 was tested).
* Header files for glib and libpcap libraries.
* Linux kernel headers (for tun/tap). They need to match the running kernel, otherwise strange things can happen.
* Make tool ([GNU Make](http://www.gnu.org/software/make/make.html) 3.80 was tested).
* [Pkg-config](http://pkgconfig.freedesktop.org/wiki/) tool (version 0.15.0 was tested).
* Install tool, part of [coreutils](http://www.gnu.org/software/coreutils/) (version 5.2.1 was tested).

#### Compilation and installation

Optionally alter instalation paths at the beginning of Makefile.

If you want to link multispoof components with libpcap dynamically (you should do this if your distribution ships libpcap as a shared library like Debian 3.1) execute:

```
$ make
```

Otherwise, you need to specify `libpcap.a` file to link statically with. For example on Redhat 9 it would be placed in `/usr/lib/libpcap.a`, so you should type:

```
$ PCAP\_STATIC=/usr/lib/libpcap.a make
```

After successful compilation, issue:

```
$ su -c "make install"
```

Note that the installation is required for program to run correctly. You can uninstall it with:

```
$ su -c "make uninstall"
```

#### Usage

Multispoof requires root provileges. Before running it you should kill DHCP client. Multispoof removes IP address from the network interface (it is reassigned on program termination). While operating, program requires no IP address set on network interface. The DHCP client could assign it, which is not desirable – and that's why it should be killed. Also, if you don't want to be caught easily by network administrator, you should change your IP and MAC addresses, because they are used for scanning.

Passing -h option to multispoof makes it show usage information. To launch the program and instruct it to use eth0 interface, invoke:

```
multispoof -i eth0
```

If everything is ok, you should see something like this:

```
netdb: Listening on /tmp/multispoof.XX410BZG/socket
rx (tapio): listening on eth0
rx (deta): listening on eth0
tapio: virtual interface: tap0
tx (tapio): using device eth0
tx (deta): using device eth0
tx (scanarp): using device eth0
cmac (unspoof): Using be:70:8d:4d:6a:a9 as default mac
```

After first run, multispoof starts learning network addresses and save them to its database file. This file is reused on consecutive runs, so gathered addresses are not lost after program termination. Multispoof uses only inactive addresses for spoofing, so it's best to leave it running for some time (for example one night). After that database should be filled with enough addresses, and hopefully some of them will be inactive. Remember, that multispoof's default behavior is waiting for 5 minutes before inactive address is to be used. So after launching of the program, there is at least that delay before Internet connection could be used.

Now launch your favorite p2p program or download accelerator and get some data really fast :) **The more file chunks are downloaded in parallel, the higher throughput you get.**

Before starting download, you should tune TCP settings in the kernel, particularly keep-alive options, for better resistance to transmission drops. It can be done with /proc files modification, or via sysctl utility.

```
sysctl -w net.ipv4.tcp_keepalive_intvl=5
sysctl -w net.ipv4.tcp_keepalive_probes=3
sysctl -w net.ipv4.tcp_keepalive_time=10
sysctl -w net.ipv4.tcp_syn_retries=0
```

You can put all of the above options in a file and load it using the -p switch. Note that keep-alive is an optional feature of TCP stack, and not all network applications are using it. Some of them implement keep-alive themselfes (ssh for example), other simply don't. For testing I've used [prozilla](http://prozilla.genesys.ro/) download accelerator patched to enable keep-alive (well, the patch actually uncomments some code). The patch is against prozilla 1.3.7.4, and can be found in the multispoof tarball (prozilla-1.3.7.4.patch). Bootable CDROM contains prozilla already patched.

**Warning:** provided sysctl settings are suitable for use when working with local machine. If you are running multispoof remotely, consider setting `keepalive_time` to a higher value. If you don't do this, you will probably experience frequent ssh disconnections.

#### Internal state monitoring

Collected addresses can be listed by connecting to multispoof address database through unix socket. You can do it with multispoof-dump tool:

```
$ multispoof-dump /tmp/multispoof.XX410BZG/socket

192.168.0.192 00:c0:df:ae:de:44 2 1122365430 disabled idle
192.168.0.76 00:30:4e:28:c0:fe 310 2 enabled idle
192.168.0.199 00:08:0d:c4:22:14 30 1122365430 disabled idle
192.168.0.33 00:30:84:42:b1:9c 41 1122365430 disabled idle
192.168.0.15 00:0a:cd:00:15:30 492 184 enabled idle
```

Assuming default value of -a option (300 seconds), addresses currently used for spoofing can be displayed with following pipeline:

```
$ multispoof-dump | awk '{if ($5 == "enabled" && $3 >= 300) print $0}'

192.168.0.85 00:30:4e:28:c3:22 310 2 enabled idle
192.168.0.15 00:0a:cd:00:15:30 492 184 enabled idle
```

To check if load balancing works correctly, you can run ping and use tcpdump to sniff its traffic:

```
$ tcpdump -eni eth0 icmp

tcpdump: WARNING: eth0: no IPv4 address assigned
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth0, link-type EN10MB (Ethernet), capture size 96 bytes
23:21:27.997833 00:20:ed:b6:78:43 > 00:20:af:c4:6f:e3, ethertype IPv4 (0x0800), length 98: IP 192.168.1.23 > 212.77.100.101: icmp 64: echo request seq 1
23:21:28.039748 00:20:af:c4:6f:e3 > 00:20:ed:b6:78:43, ethertype IPv4 (0x0800), length 98: IP 212.77.100.101 > 192.168.1.23: icmp 64: echo reply seq 1
23:21:28.992051 00:02:44:7b:09:11 > 00:20:af:c4:6f:e3, ethertype IPv4 (0x0800), length 98: IP 192.168.1.42 > 212.77.100.101: icmp 64: echo request seq 2
23:21:29.036361 00:20:af:c4:6f:e3 > 00:02:44:7b:09:11, ethertype IPv4 (0x0800), length 98: IP 212.77.100.101 > 192.168.1.42: icmp 64: echo reply seq 2
23:21:29.992307 00:20:ed:b6:78:43 > 00:20:af:c4:6f:e3, ethertype IPv4 (0x0800), length 98: IP 192.168.1.23 > 212.77.100.101: icmp 64: echo request seq 3
23:21:30.046712 00:20:af:c4:6f:e3 > 00:20:ed:b6:78:43, ethertype IPv4 (0x0800), length 98: IP 212.77.100.101 > 192.168.1.23: icmp 64: echo reply seq 3
23:21:30.994180 00:02:44:7b:09:11 > 00:20:af:c4:6f:e3, ethertype IPv4 (0x0800), length 98: IP 192.168.1.42 > 212.77.100.101: icmp 64: echo request seq 4
23:21:31.098238 00:20:af:c4:6f:e3 > 00:02:44:7b:09:11, ethertype IPv4 (0x0800), length 98: IP 212.77.100.101 > 192.168.1.42: icmp 64: echo reply seq 4
```

#### Bugs

There is a bug in connectivity testing. Sometimes DNS packets are dropped by multispoof, and user gets "dns problem" message. There is no fix yet (patches are welcomed!), but a workaround.

Edit access-test script, comment the code that resolves domain name and set HOST variable to the IP address of <www.google.com> (for example).

#### FAQ

1. I get following error on multispoof compilation:

    rx.c:97: error: \`PCAP\_D\_IN' undeclared (first use in this function)

    You need more recent libpcap. See [requirements](#requirements).

2. When I run multispoof following message appears:

    multispoof: Your system doesn't support required iptables features.

    Well, it means your system doesn't support required iptables features. See [requirements](#requirements) for details. Hint: Try to specify -v switch to see what features are missing.

3. Does multispoof run on anything other than Linux?

    No. It requires Linux-specific kernel features. Of course it can be ported, but it isn't and probably won't be.

4. Does multispoof work on cable/dial-up/dsl/whatever?

    Multispoof will do it's job on Ethernet-based, mac-spoofing-vulnerable networks only. Cable networks are protected against mac-spoofing, but if you want to learn about possible abuses in this type of networks type _uncapping_ in google.

5. Do you plan to make multispoof run on > 2.6.11 kernels? Could you add feature X?

    No, I don't plan to work on multispoof anymore.

6. I don't know anything about networks and Linux. I just want to steal some bandwidth from my ISP. Make multispoof work for me!

    Sure, but first have a look [here](http://en.wikipedia.org/wiki/Script_kiddies).

#### Author, contact

This software was written by me, Paweł Pokrywka. You can find my email address here:
[https://blog.pawelpokrywka.com](https://blog.pawelpokrywka.com)
