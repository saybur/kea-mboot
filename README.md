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
- Files from this repository,
- Functional networking for these devices.

I'll go through some of these items in a bit more detail.

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
You will either need this file or correctly-formatted equivalents.

## Setup DHCP Server (Kea)

> [!NOTE]
> DHCP is a broadcast protocol and by default Kea will interfere with any
> existing DHCP server. I personally keep all my retro systems on a separate
> layer 2 VLAN using Kea as the DHCP server, and I have not needed to
> experiment with ways to integrate with an existing service. You could
> potentially try to filter by MAC addresses if your existing server allows it
> (see [here](https://www.mail-archive.com/kea-users@lists.isc.org/msg04380.html)
> for Kea-side configuration), but I do not know how well that would work in
> practice. If you do get something like that working feel free to send a PR
> with instructions.

Kea has a powerful configuration system based on JSON. Unfortunately, there are
quirks to the NetBoot protocol that require a custom hook to make things work.

Install Kea, along with the building requirements for the hook.

```
sudo apt install kea-dhcp4-server kea-dev libboost-dev build-essential git
```

Then clone this repo with `git`. A simple `make` will produce the needed shared
library. Run `sudo make install` to drop it into the Kea hooks library folder.

> [!NOTE]
> Starting with 2.6.3, Kea will only load hooks libraries from a single
> directory, defined at compile time. For details refer to
> <https://kea.readthedocs.io/en/kea-2.6.3/arm/hooks.html>. The Makefile will
> try to auto-resolve this but if you run into trouble you may need to manually
> redefine KEA_USER_HOOKS.

If you're looking for copy/paste instructions something like this should work.

```
mkdir -p ~/src
cd ~/src
git clone https://github.com/saybur/kea-mboot.git
cd kea-mboot/src
make
sudo make install
```

All this hook does is re-order the BOOTP options, which _appears_ to be
required or the Mac will refuse to boot. I couldn't figure out how to do this
natively in Kea.

The configuration file lives in `/etc/kea/kea-dhcp4.conf`. A full example
configuration file is in this repo. You will need to tweak settings for your
specific network setup (interface assignment, address range, yada yada).
The following sections are important to get network booting operational.

### `"option-def"`

This portion defines the structure of the various netboot options. Include
it verbatim in your configuration file. The various "code" entries correspond
to the BOOTP options used by NetBoot 1.0, which are described in more detail
here:

<http://web.archive.org/web/20030316193425/http://mike.passwall.com/macnc/analysis.html>

### `"client-classes"`

This defines the test for a NetBoot client and the response given back. Most
values should not be changed from the example, except for the following:

- "next-server" is the TFTP server's IP address (I've only tested this when it
  is the same as the Netatalk server IP).
- "boot-file-name" is the TFTP server path where the ROM resides (see later
  section for TFTP configuration).
- "server-hostname" is the Netatalk server's IP address.
- "broadcast-address" should match the network's value.
- "netboot-user" is the Netatalk user accessing the share.
- "netboot-pass" is the Netatalk user's password.
- "netboot-client" is the directory where the client's writable image resides.
- "netboot-img-boot" is the bootable hard drive image (may be read only), using
  a combination of several values:
    - IP address of the AppleTalk server,
    - network port to connect to,
    - share name,
    - "0" (do not change),
    - "2" (do not change),
    - The full path to the relevant image, using `\u0000` as the path
      separator.
- "netboot-img-apps" is the applications image, using the format above, also
  possibly read-only.
- "netboot-img-client" is the writeable portion of the image for the specific
  connecting client, again using the above format, and probably residing in a
  directory matching "netboot-client" above.

It should be possible to override the most important per-client values
("netboot-client" and "netboot-img-client") via Kea static leases or other
such approaches, but since I only have one system I'm trying to boot I haven't
experimented with how to configure that.

### `"hooks-libraries"`

The example file enables both the required BOOTP support and the custom hook
that handles reordering the values for NetBoot. Include this verbatim.

> [!NOTE]
> The hook will look for the `netboot` class name and won't modify packets
> unless it is present. That said, I don't have other BOOTP clients to test
> with so you might still encounter weird cases: please report if you do.

### Finish Up

I recommend reading the writeup at
<http://web.archive.org/web/20030207040746/http://mike.passwall.com/macnc/>
to understand more about what the options themselves do. In 2025, Kea allows
this to be cleaner (or at least leave out the hexadecimal ASCII).

Start Kea and enable it after each boot.

    sudo systemctl enable kea-dhcp4-server
    sudo systemctl start kea-dhcp4-server

I've found Kea's configuration syntax is a bit quirky, check the logs carefully
for errors prior to proceeding further.

    sudo journalctl -u kea-dhcp4

## Setup Netatalk Server

Starting with Debian _trixie_ Netatalk 4.0 is included in the main repositories.
Much appreciation to the Netatalk maintainers, they're doing a fantastic job
keeping this venerable package running on modern systems!

Install Netatalk.

    sudo apt install netatalk

Set up the share for the Mac to connect to by adding the following to
`/etc/netatalk/afp.conf`

```
[NetBootVol]
path = /srv/netboot
valid users = netboot
ea = ad
```

> [!NOTE]
> You only _need_ `ea = ad` if you're using Option 2 below for handling the
> resource forks. Read the Netatalk documentation on this point if uncertain.

Create the user that will be connecting to this share.

    sudo adduser --system --group --shell /usr/sbin/nologin netboot

Use `sudo passwd netboot` and assign the password `12345lol` (or whatever you
changed the relevant field to in `kea-dhcp4.conf`).

> [!CAUTION]
> This has obvious security implications. Use care to ensure this user cannot
> log into the system.

Create the location being served under your main user account.

```
sudo mkdir /srv/netboot
sudo chown $USER:$GROUP /srv/netboot
mkdir /srv/netboot/client
```

If in use, add a firewall rule allowing port 548/TCP. Once done, enable and
(re)start the server.

    sudo systemctl stop netatalk
    sudo systemctl enable netatalk
    sudo systemctl start netatalk

## TFTP Server

Install a TFP server. Any should work but this assumes `tftpd-hpa`, which seems
to work well for this purpose (_not_ `tftp-hpa` the client).

    sudo apt install tftpd-hpa

On Debian this package starts by default with a sane config, serving
`/srv/tftp`.

## Setup Files

You can extract `NetBoot9.dmg` using utilities on a modern system. While `7z`
works fine for many DMG files this one required `dmg2img` to get the process
started. Install both tools:

    sudo apt install dmg2img p7zip-full

Translate and unpack the file containing the images to a convenient location.

```
dmg2img NetBoot9.dmg NetBoot9.img
7z e NetBoot9.img "NetBoot for Mac OS 9/English/NetBoot.pkg/Contents/Resources/NetBoot.pax.gz"
gzip -d NetBoot.pax.gz
7z x NetBoot.pax
chmod +x NetBootInstallation
```

Inside the `NetBootInstallation` folder there are three images and their
corresponding resource forks in `._` notation. Place copies of the main files
into the Netatalk share.

```
cp "NetBootInstallation/NetBoot HD.img" /srv/netboot/NetBoot.img
cp "NetBootInstallation/Applications HD.img" /srv/netboot/Applications.img
cp "NetBootInstallation/Applications HD.img" /srv/netboot/client/User.img
```

> [!NOTE]
> This isn't a typo: the first two files are the main images and the third is a
> scratch image, which in this case is just a copy of the applications image.

From this point you have three different paths to take depending on how you
want to handle the resource forks. _Only use one of these approaches!_

### Option 1: Upload Files

If you have access to a system that can correctly handle the dot files in the
`.pax` archive and has an AFP client (like OSX) you can just unpack and upload
the images after configuring the share appropriately. If you do this, delete
the files copied in the previous step prior to uploading new ones, taking care
to preserve the paths exactly as shown.

### Option 2: .AppleDouble folders/files

This approach does not require another Mac and can be done from the server.
`.AppleDouble` folders are used with the resource fork added to files with the
hacky `admerge` script I put together. Build that first.

    gcc ~/src/kea-mboot/util/admerge.c -o ~/src/kea-mboot/util/admerge

Have Netatalk generate blank AppleDouble files for these with the following
command.

    sudo dbd -f /srv/netboot

This may produce a bunch of messages the first time but a subsequent run
should output nothing.

Use the `admerge` utility to inject the resource fork into these blank files.

```
~/src/kea-mboot/util/admerge "NetBootInstallation/._NetBoot HD.img" /srv/netboot/.AppleDouble/NetBoot.img
~/src/kea-mboot/util/admerge "NetBootInstallation/._Applications HD.img" /srv/netboot/.AppleDouble/Applications.img
~/src/kea-mboot/util/admerge "NetBootInstallation/._Applications HD.img" /srv/netboot/client/.AppleDouble/User.img
```

Rebuild the database again.

    sudo dbd -f /srv/netboot

Assign write permissions to the netboot user for the client files.

    sudo chown -R netboot:netboot /srv/netboot/client

### Option 3: Extended Attributes (Untested)

The alternate way to do this with Netatalk is via the built-in `ad` utility,
extended attributes, and the dot files (`._`). I was not able to get this
working but I wouldn't be shocked if it was just due to errors on my part. If
you would like to try, see the posts I made on 68kmla while I bumbled my way
through the process:

<https://68kmla.org/bb/index.php?threads/netatalk-4-0-future-proofing-apple-file-sharing.47958/post-571137>

Also be sure to remove `ea = ad` in the Netatalk share configuration.

### ROM

Regardless of how you handle Netatalk you'll still need the ROM in TFTP. Copy
`Mac OS ROM` into a `boot` subfolder of the TFTP share such that the final path
on disk is `/srv/tftp/boot/MacROM`. This will correspond to the Kea
configuration example.

    sudo mkdir -p /srv/tftp/boot
    sudo cp "NetBootInstallation/Mac OS ROM" /srv/tftp/boot/MacROM

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
  permissions (or similar troubles). This can be done by adding a line like
  `log level = default:debug` under the `[Global]` section in `afpd.conf` and
  watching with `sudo journalctl -fu netatalk`.

## References

1. <http://web.archive.org/web/20100616031251/http://frank.gwc.org.uk/~ali/nb/>
2. <http://web.archive.org/web/20030207040746/http://mike.passwall.com/macnc/>
3. <https://opensource.apple.com/source/bootp/bootp-268.1/Documentation/BSDP.doc>
4. <http://web.archive.org/web/20070302173932/http://www.macos.utah.edu/documentation/system_deployment/netboot.html>
