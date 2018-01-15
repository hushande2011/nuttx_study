#!/bin/bash

if [[ "$1" = "-b" ]] ; then
    openocd -f interface/stlink-v2.cfg -f target/stm32f1x_stlink.cfg -c init -c "reset halt" -c " flash write_image erase nuttx.bin 0x08004000 " -c "reset run" -c exit
elif [[ "$1" = "-m" ]] ; then
	cd tools
	PROJECT_NAME="m5"
	if [[ "$2" = "m5" ]] ; then
	PROJECT_NAME="m5"
	fi
	if [[ "$2" = "nsh" ]] ; then
	PROJECT_NAME="nsh"
	fi
	echo "./configure.sh fire-stm32v2/$PROJECT_NAME"
	./configure.sh fire-stm32v2/$PROJECT_NAME
	cd -
	make
elif [[ "$1" = "-mm" ]] ; then
	cd tools
	echo "./configure.sh fire-stm32v2/$2"
	./configure.sh fire-stm32v2/$2
	cd -
	make
    cat bootloader-16k.bin nuttx.bin > nuttx_full.bin
elif [[ "$1" = "-mk5" ]] ; then
	cd tools
	echo "./configure.sh fire-stm32v2/c5_nsh"
	./configure.sh fire-stm32v2/c5_nsh
	cd -
	make
    cat bootloader-16k.bin nuttx.bin > nuttx_full.bin
elif [[ "$1" = "-my" ]] ; then
	cd tools
	echo "./configure.sh stm32f4discovery/my_nsh"
	./configure.sh stm32f4discovery/my_nsh
	cd -
	make
   # cat k9_head.bin nuttx.bin > nuttx_full.bin
elif [[ "$1" = "-c" ]] ; then
	make distclean
else
    make
    cat bootloader-16k.bin nuttx.bin > nuttx_full.bin
fi
