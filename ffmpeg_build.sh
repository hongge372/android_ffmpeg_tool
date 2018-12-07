#chg by wanglizheng on 2018.7.17
#make ffmpeg static libraries, including fdkaac and x264 libs.

# Created by jianxi on 2017/6/4
# https://github.com/mabeijianxi
# mabeijianxi@gmail.com

#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
#export TMPDIR=$basepath/ffmpegtemp
# NDK的路径，根据自己的安装位置进行设置

echo $basepath

FF_EXTRA_CFLAGS=" -fPIC -ffunction-sections -funwind-tables -fstack-protector -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -fomit-frame-pointer -fstrict-aliasing -funswitch-loops -finline-limit=300 "
#-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16


FF_CFLAGS="-O3 -Wall -pipe \
-ffast-math \
-fstrict-aliasing -Werror=strict-aliasing \
-Wno-psabi -Wa,--noexecstack \
-DANDROID  "


FF_EXTRA_CFLAGS=""
FF_CFLAGS=""

rm "./compat/strtod.o"

build_one(){
    ./configure \
	  --prefix=install \
	  --disable-stripping \
	  --enable-gpl \
	  --enable-version3 \
	  --enable-pthreads \
	  --enable-small \
	  --disable-iconv \
	  --enable-encoders \
	  --enable-neon \
	  --enable-x86asm \
	  --enable-hwaccels \
	  --enable-encoder=mjpeg \
	  --enable-encoder=png \
	  --enable-nonfree \
	  --enable-muxers \
	  --enable-muxer=mov \
	  --enable-muxer=mp4 \
	  --enable-muxer=h264 \
	  --enable-muxer=avi \
	  --enable-decoders \
	  --enable-decoder=aac \
	  --enable-decoder=aac_latm \
	  --enable-decoder=h264 \
	  --enable-decoder=mpeg4 \
	  --enable-decoder=mjpeg \
	  --enable-decoder=png \
	  --enable-decoder=mp3 \
	  --enable-demuxers \
	  --enable-demuxer=image2 \
	  --enable-demuxer=h264 \
	  --enable-demuxer=aac \
	  --enable-demuxer=avi \
	  --enable-demuxer=mpc \
	  --enable-demuxer=mpegts \
	  --enable-demuxer=mov \
	  --enable-demuxer=mp3 \
	  --enable-demuxer=concat \
	  --enable-parsers \
	  --enable-parser=aac \
	  --enable-parser=ac3 \
	  --enable-parser=h264 \
	  --enable-protocols \
	  --enable-zlib \
	  --enable-avfilter \
	  --disable-outdevs \
	  --enable-ffprobe \
	  --enable-ffmpeg \
	  --enable-debug \
	  --enable-postproc \
	  --enable-avdevice \
	  --disable-symver \
	  --disable-doc \
	  --disable-stripping \
	  --extra-cflags="$FF_EXTRA_CFLAGS  $FF_CFLAGS" \
	  --extra-ldflags="  "
}
build_one


make clean
make -j16
#make install

cp $FDK_LIB/libfdk-aac.so $PREFIX/lib

