#!/bin/bash

start=0x$1
end=0x$2
echo arm-none-linux-gnueabi-objdump -DS vmlinux --start-address=$start --stop-address=$end
arm-none-linux-gnueabi-objdump -DS vmlinux --start-address=$start --stop-address=$end


