#!/bin/bash

./configure --enable-gpl --enable-nonfree --enable-asm \
	    --extra-cflags="-I$X264_INSTALL/include -I$X265_INSTALL/include" \
	    --extra-ldflags="-L$X264_INSTALL/lib -L$X265_INSTALL/lib" \
	    --enable-libx264 \
	    --enable-libx264 \
	    --enable-debug

make -j4
