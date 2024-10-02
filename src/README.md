kea-mboot library
=================

A Kea hooks library for re-writing parts of BOOTP packets to conform to the
expectations of early New World ROM net-boot programs.

This library is a cludge/hack in two ways. First, the Macs appear to need the
options in a particular order or they will refuse to boot, which is weird.
Second, this code is a bit of a mess and could use some cleanup. It is heavily
derived from the MPL2 licensed BOOTP library distributed with Kea and retains
that licensing.

The specific order of options is based on what I observed from a working packet
emitted from the ISC DHCP 3.0 server after the patch by Rob Lineweaver. Source:

<http://web.archive.org/web/20030405043314/http://staff.harrisonburg.k12.va.us/~rlineweaver/macnb/>

This works fine for me on my network so I'm putting it out there for others to
use and/or build upon. As with the parent folder, suggestions or improvements
are welcome.
