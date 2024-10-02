Netboot 1.0 for Retro Enthusiasts
=================================

This page documents my "adventure" trying to get a 1998 Rev B iMac net-booting
OS 9 via modern (2024) open source server software. This includes a custom Kea
DHCP server hook, along with some configuration examples and general hints.

This guide assumes a lot, mainly familiarly with Linux, old Macs, and
networking more generally. If that sounds intimidating you may want to consider
a different approach than net booting, it is kind of a PITA. With that said, I
hope this can be the basis for something that an out-of-the-box project can
integrate to make it easier. Barring that, I welcome input (including PRs) to
fix/amend/clarify/whatever anything here. My goal is to let this serve as a
template for others to net-boot their systems with fewer headaches than I had
trying to get it working.

Most of the credit here belongs to Michael Egan, Rob Lineweaver, and many
others, who got this working in production environments around the turn of the
millenium and did a great job documenting their work. Major thanks to all of
them, this would have been impossible to figure out without their efforts.

## Raison d'etre

I have a 1998 Rev B iMac without a working CD-ROM drive. Apple rather famously
made these systems without many external bus options and Open Firmware wasn't
cooperating with attempts to boot from USB. Rather than do sensible things,
like fix/replace the CD drive or pull the hard drive for installation on
another computer, I foolishly thought, "why not use this fancy NetBoot thing to
do the OS installation, I mean, _how hard could it be_?"

## You Will Need These Things

- NetBoot 1.0 compatible Mac computer,
- Modern Linux/BSD server, with services,
- A Mac ROM and disk images for your server,
- Working network for all these devices.

I'll go through each of these in a bit more detail.

### Compatible Systems

This process only works with the BOOTP based NetBoot 1.0, which is implemented
on the following New World ROM systems:

- some iMacs (tray-loaders),
- some early iBooks,
- Power Mac G3s and the Yikes! based G4,
- Lombard PowerBooks.

Later models use a newer DHCP-based approach called BDSP (NetBoot 2.0). This is
better documented and there are open source options that could potentially
work, like <https://github.com/bruienne/bsdpy> and possibly others. Based on
the limited information I could find I think those newer systems basically
_require_ a BDSP server to work (at least I couldn't convince my Summer 2000
iMac DV+ to boot after getting the original iMac working).

### Server Software

This all assumes the server will run Debian 12 (bookworm) but the basic process
should work with any modern Linux/BSD system, you'll just need to tweak things
a bit. The following pieces of software are needed:

- Kea DHCP server (<https://www.isc.org/kea/>),
- Netatalk (<https://netatalk.io/>,
- a TFTP server

Netatalk is undergoing a renaissance thanks to hard work by the estimable
@rdmark, @NJRoadfan, and others. I've been using Netatalk 2 for ages but
Netatalk 4.0 is brand-new as of this writing and includes the AppleTalk support
obsoleted in Netatalk 3. It also introduces the 3.0 configuration syntax, which
I am not familiar with, which makes config file examples tricky to give. For
now, you can follow
[their guide](https://netatalk.io/docs/Installing-Netatalk-2-on-Debian-Linux)
for how to build and install it on Debian and I'll try to get examples updated
once I give Netatalk 4 a spin.

The remaining packages are in the Debian repos and are just an `apt get` away!
:)

### Images

Apple used to make images available in a `NetBoot9.dmg` file, see
<https://systemfolder.wordpress.com/2020/02/11/netboot-to-rescue/> for details.
If you have a copy, getting the images out of it is an exercise left for the
reader. Bear in mind you _must_ preserve both the data and resource forks of
the image files or they will not work!

The user image can just be a copy of the applications image or a new Disk Copy
6.3 image you create. Remember anything you make can be placed in a MacBinary
II `.bin` file and unpacked on a Netatalk server via the _megatron_ `unbin`
tool, which will properly restore the resource fork.

## Server Setup

In broad strokes, you need Netatalk set up as follows:

- AFP over IP enabled and working (default port 548/TCP),
- A user `netboot` created with a password of `12345lol`,
- A server share called `NetBootVol` to house files,
- `NetBootDir/NetBoot HD.img` created, read-only permissions,
- `NetBootDir/Applications HD.img`, read-only permissions,
- `NetBootDir/imac_revb/User.img` read/write permissions for the
  `netboot` user (along with the `imac_revb` containing folder).

The Kea config example assumes the above setup; obviously much of this can be
tweaked once you verify it works.

Install a TFP server, like `tftpd-hpa`. On Debian that package has a sane
default config and sets up `/srv/tftp` as the hosting directory. Copy
`Mac OS ROM` into a `boot` subfolder such that the final path on disk is
`/srv/tftp/boot/MacROM`.

### Kea

Kea has a really powerful configuration system based on JSON. Unfortunately,
there are some quirks to the NetBoot protocol that require a custom hook be
installed. Get Kea and the building requirements for the hook:

```
sudo apt install kea-dhcp4-server kea-dev libboost-dev build-essential git
```

Then clone this repo with `git`. A simple `make` should produce the needed
shared library, which you can install with `sudo make install` to put it into
`/usr/local/lib/kea/hooks/mboot-hook.so`.

By default, AppArmor prevents this library from loading. Edit
`/etc/apparmor.d/usr.sbin.kea-dhcp4` and add the following line before the
ending brace (don't omit the comma).

```
/usr/local/lib/kea/hooks/mboot-hook.so rm,
```

All this hook does is re-order the BOOTP options, which _appears_ to be
required or the system will refuse to boot. I couldn't figure out how to do
this natively in Kea.

The configuration file lives in `/etc/kea/kea-dhcp4.conf`. An example config
file is in this repo. I strongly recommend reading the writeup at
<http://web.archive.org/web/20030207040746/http://mike.passwall.com/macnc/>
to understand more about what the options themselves do. In 2024, Kea allows
this to be cleaner, at least omitting the hexadecimal ASCII.

Make sure you restart Kea after getting everything set up.

## Booting

Connect your old Mac to the network and boot it up, holding down the _N_ key as
the system starts. If all goes well you should be greeted with a happy Mac and
OS 9 will begin to (slowly) start.

If using the above `NetBoot9.dmg` images you'll be asked to connect to a
Macintosh Manager server. Declining will ask you for an admin user and
password. The username is `NBUser` and the password is `netboot`.

## Troubleshooting

If it wasn't obvious from the text above, this process is obnoxious and prone
to failing at many different points. If you run into issues, here are some
potentially useful hints:

- If you get a cursor it is a good indication the TFTP ROM download went fine.
  Otherwise, check the TFP side of things (and that your BOOTP packet looks
  correct, Wireshark might help with that).
- The netboot client on the Macs gives you no indication of what went wrong
  during bootup. You will need server logs, particularly from `afpd`. Suggest
  turning on debug logging, which will helpfully emit helpful errors like
  `AFP_ERR_ACCESS` when the client tries to fetch a file with incorrect
  permissions (or similar troubles).

## References

1. <http://web.archive.org/web/20100616031251/http://frank.gwc.org.uk/~ali/nb/>
2. <http://web.archive.org/web/20030207040746/http://mike.passwall.com/macnc/>
3. <https://opensource.apple.com/source/bootp/bootp-268.1/Documentation/BSDP.doc>
4. <http://web.archive.org/web/20070302173932/http://www.macos.utah.edu/documentation/system_deployment/netboot.html>
