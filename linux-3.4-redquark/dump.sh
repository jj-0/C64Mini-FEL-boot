#!/bin/bash
start=`grep " "$1"$" System.map | awk '{print "0x"$1}'`
end=`grep -A 1 " "$1"$" System.map | awk '{getline; print "0x"$1}'`
echo "routine $1, star address $start, end address $end"

#if [ "$start"="$end" ]; then
#start=`expr "16#$end" + 4`
#echo "routine $1, star address $start, end address $end"
#fi
echo arm-none-linux-gnueabi-objdump -DS vmlinux --start-address=$start --stop-address=$end
arm-none-linux-gnueabi-objdump -DS vmlinux --start-address=$start --stop-address=$end


