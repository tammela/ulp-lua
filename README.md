ULP Lua
========

An upper layer protocol for Lua scripting.

## Description

ULP Lua is a kernel module that provides scripting capabilites for the in-kernel
socket structure. It achieves this by using a modified version of the Lua 5.3
virtual machine that runs in kernel mode.

ULP Lua is dependant on a new feature introduced in the Linux kernel called
upper layer protocol (ULP). This new feature allow the user to attach L4
capabilites and extend the in-kernel socket. It's the bones used on mainline in
TLS in Kernel.

ULP was merged in the 4.1x series, but the patch is trivial and can be backported.

## Usage

ULP Lua can be used in a number of different applications, such as zero copy
L4 in-kernel introspection, WAFs, proxies, etc...

ULP Lua has a design goal to work directly on skbs and leverage zero copy
whenever it's possible.

## Known Limitations

Current known limitations are listed below and are intented to be fixed in the
future.

--> *No* support for fragmented skbs.

--> *No* support for sparse buffer pattern matching.

--> *No* Lua API for common socket operations.

--> *No* support for IPv6.

--> *No* support for udp.

## Contributing

ULP Lua is licensed as GPL v2.

You can contribute to this project via pull requests and issues.

The directory structure is:

`dependencies` -- where all the dependencies are, usually in submodules.

`src` -- where the kernel module sources are.

`tests` -- where all the tests programs and resource validators are.

`docs` -- where all the in-depth documentations are.

The code contains helpful comments as well, at least for me :).

## Notice

ULP Lua is a project born out of my bachelor's thesis at PUC-Rio.

Since Linux 4.20 the ULP infrastructure has changed.
Now we require a kernel >= 4.20.

If you wish to use an older version, please backport `module.c`.
