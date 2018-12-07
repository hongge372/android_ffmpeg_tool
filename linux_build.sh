#!/bin/bash

./configure --extra-cflags='-I/qiche/cmd-rotate-scale-0821/cmd-android-ffmpeg/FFmpeg/ffmpeg/libyuv/include'  --extra-ldflags='-L/qiche/cmd-rotate-scale-0821/cmd-android-ffmpeg/FFmpeg/ffmpeg/libyuv -lyuv'


make clean
make


#./configure --extra-cflags='-I/qiche/cmd-rotate-scale-0821/cmd-android-ffmpeg/FFmpeg/ffmpeg/libyuv/include'  --extra-ldflags='-L/qiche/cmd-rotate-scale-0821/cmd-android-ffmpeg/FFmpeg/ffmpeg/libyuv -l/qiche/cmd-rotate-scale-0821/cmd-android-ffmpeg/FFmpeg/ffmpeg/libyuv.a'
