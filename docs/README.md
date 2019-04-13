ULP Lua
========

## Setup

We recommend a Linux setup with QEMU+KVM and virt-manager. You may still use vagrant
for automatically setting up the virtual machine and the environment.

As our guest Linux distribution we recommend a rolling release distribution
like OpenSUSE Tumbleweed or Arch Linux in a minimal installation. This way
we can keep up with the stable Linux release cycle as soon as they are available from the official repositories.

The packages we recommend are:
```
        build-essentials (make, gcc...)
        command line editor (vim, emacs...)
        git
        sshd
        curl
        netcat
```

You may still use Ubuntu or others non-rolling release distributions. Ubuntu has a mainline kernel installation guide,
from pre-compiled binaries, [here](https://wiki.ubuntu.com/Kernel/MainlineBuilds).
