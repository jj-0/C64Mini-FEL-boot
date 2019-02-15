#!/bin/sh

echo "insmod ump.ko:\n"
insmod ump.ko
echo ""

echo "insmod mali.ko:\n"
insmod mali.ko
echo ""

echo "run gles2_api_suite:\n"
#./gles2_api_suite
./gles2_api_suite --suite="glDepthFunc" --test="AGE05-glDepthFunc"
echo ""
