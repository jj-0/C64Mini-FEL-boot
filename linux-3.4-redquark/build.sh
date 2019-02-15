# build.sh
#
# (c) Copyright 2013
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# Notice:
#   1. Board option is useless.

if [ $(basename `pwd`) != "linux-3.4" ] ; then
    echo "Please run at the top directory of linux-3.4"
    exit 1
fi

. ../buildroot/scripts/shflags/shflags

# define option, format:
#   'long option' 'default value' 'help message' 'short option'
DEFINE_string 'platform' 'sun7i' 'platform to build, e.g. sun7i' 'p'
DEFINE_string 'board' '' 'board to build, e.g. evb' 'b'
DEFINE_string 'module' '' 'module to build, e.g. kernel, modules, clean' 'm'

# parse the command-line
FLAGS "$@" || exit $?
eval set -- "${FLAGS_ARGV}"

LICHEE_CHIP=${FLAGS_platform%%_*}
LICHEE_PLATFORM=${FLAGS_platform##*_}
LICHEE_BOARD=${FLAGS_board}

if [ "${LICHEE_CHIP}" = "${LICHEE_PLATFORM}" ] ; then
    LICHEE_PLATFORM="linux"
fi

#tooldir="$(dirname `pwd`)/out/${LICHEE_PLATFORM}/common/buildroot/external-toolchain"
tooldir="/home/chris/src/buildroot/buildroot/output/host/usr"
if [ -d ${tooldir} ] ; then
    if ! echo $PATH | grep -q "$tooldir" ; then
        #export PATH=${tooldir}/bin:$PATH
        export PATH=${tooldir}/bin:$PATH
    fi
else
    echo "Please build buildroot first"
    exit 1
fi

if [ ${LICHEE_PLATFORM} = "linux" ] ; then
    build_script="scripts/build_${LICHEE_CHIP}.sh"
    LICHEE_KERN_DEFCONF="${LICHEE_CHIP}smp_defconfig"
else
    build_script="scripts/build_${LICHEE_CHIP}_${LICHEE_PLATFORM}.sh"
    LICHEE_KERN_DEFCONF="${LICHEE_CHIP}smp_${LICHEE_PLATFORM}_defconfig"
fi

export LICHEE_CHIP
export LICHEE_PLATFORM
export LICHEE_BOARD
export LICHEE_KERN_DEFCONF

LICHEE_TOOLS_DIR="$(dirname `pwd`)/tools"
LICHEE_PLAT_OUT="$(dirname `pwd`)/out/${LICHEE_PLATFORM}/common"
LICHEE_BR_OUT="${LICHEE_PLAT_OUT}/buildroot"
mkdir -p ${LICHEE_BR_OUT}

export LICHEE_TOOLS_DIR
export LICHEE_PLAT_OUT
export LICHEE_BR_OUT

if [ -x ./${build_script} ]; then
    ./${build_script} ${FLAGS_module}
else
	echo "ERROR: please check the following points:"
    echo "    1. platform to build. (-p)"
    echo "    2. ${build_script} exist?"
    echo "    3. ${build_script} executable?"
	exit 1
fi
