Netboot 1.0 for Retro Enthusiasts
=================================

This page documents my "adventure" trying to get a 1998 Rev B iMac booting
MacOS 9 over the network via modern (2025) open source server software. This
includes a custom Kea DHCP server hook, along with some configuration examples
and general hints.

This guide assumes a lot: familiarly with Linux, old Macs, and networking more
generally. If that sounds intimidating you may want to consider a different
approach than net-booting, it is kind of a PITA. With that said, I hope this
documentation/hook will let others boot their systems off a network with fewer
headaches than I had trying to get it working, or even serve as the basis for
an out-of-the-box project to integrate.

Most of the credit here belongs to Michael Egan, Rob Lineweaver, and many
others who got this working in production environments around the turn of the
millenium and did a great job documenting their work. Major thanks to all of
them, this would have been impossible to figure out without their efforts.

I welcome input (including PRs) to fix/amend/clarify/whatever anything here.

## Raison d'etre

I have a 1998 Rev B iMac without a working CD-ROM drive. Apple famously made
these systems without many external bus options and Open Firmware wasn't
cooperating with attempts to boot from USB. Rather than do sensible things,
like fix/replace the CD drive or pull the hard drive for installation on
another computer, I foolishly thought, "why not use this fancy NetBoot thing to
do the OS installation, I mean, _how hard could it be_?"

## Things You'll Need

- NetBoot 1.0 compatible Mac computer,
- Modern Linux/BSD server,
- A Mac ROM and disk images for your server,
- Functional networking for these devices.

I'll go through each item in a bit more detail.

### Compatible Systems

This process only works with the BOOTP based NetBoot 1.0, implemented on the
following New World ROM systems:

- some iMacs (tray-loaders),
- some early iBooks,
- Power Mac G3s and the Yikes! based G4,
- Lombard PowerBooks.

Later models use a newer DHCP-based approach called BDSP (NetBoot 2.0). This is
better documented and there are open source options that could potentially
work, like <https://github.com/bruienne/bsdpy> and possibly others. Based on
the limited information I could find I think those newer systems _require_ a
BDSP server to work (and anecdotally I couldn't convince my Summer 2000 iMac
DV+ to boot after getting the original iMac working).

### Server Software

This writeup assumes the server will run Debian 13 (_trixie_) but the services
and basic process should work with any modern Linux/BSD system. The following
pieces of software are needed:

- Kea DHCP server (<https://www.isc.org/kea/>),
- Netatalk (<https://netatalk.io/>,
- a TFTP server

### Images

Apple used to make boot images available in a `NetBoot9.dmg` file, see
<https://systemfolder.wordpress.com/2020/02/11/netboot-to-rescue/> for details.
If you have a copy, you can extract the components needed using modern
utilities. While `7z` works fine for many DMG files this particular one
required the `dmg2img` to get the process started. Install both tools:

```
sudo apt install dmg2img p7zip-full
```

Unpack the file containing the images:

```
dmg2img NetBoot9.dmg NetBoot9.img
7z e NetBoot9.img "NetBoot for Mac OS 9/English/NetBoot.pkg/Contents/Resources/NetBoot.pax.gz"
```

Keep `NetBoot.pax.gz` around, you'll need it later.

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
