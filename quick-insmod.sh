#!/bin/bash
sudo insmod ./dependencies/lunatik/lunatik.ko
sudo insmod ./dependencies/luadata/luadata.ko
sudo modprobe tls
sudo insmod ./src/ulp-lua.ko
