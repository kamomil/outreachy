#!/bin/bash
set -x

if [ ! if ! md5sum -c jelly.md5 ]
then
	wget http://jell.yfish.us/media/jellyfish-10-mbps-hd-h264.mkv
fi

if [ ! -f images/jelly-1920-1080.YU12 ]
then
	ffmpeg -i jellyfish-10-mbps-hd-h264.mkv -c:v rawvideo -pix_fmt yuv420p -f rawvideo /tmp/tmp
	#reduce the video length by half, by removing the first 225 and last 255 frames.
	tail -c $((3*1920*1080*675/2)) /tmp/tmp | head -c $((3*1920*1080*450/2)) > images/jelly-1920-1080.YU12
	rm /tmp/tmp
fi
if [ ! -f images/jelly-1920-1080.RGB3 ]
then
	ffmpeg -s 1920x1080 -pix_fmt yuv420p -f rawvideo -i images/jelly-1920-1080.YU12 -pix_fmt rgb24 -f rawvideo images/jelly-1920-1080.RGB3
fi

if [ ! -f images/jelly-902-902.RGB3 ]
then
	ffmpeg -s 1920x1080 -pix_fmt rgb24 -f rawvideo -i images/jelly-1920-1080.RGB3 -filter:v "crop=902:902:0:0" -pix_fmt rgb24  -f rawvideo images/jelly-902-902.RGB3
fi

if [ ! -f images/jelly-1920-1080.NV12 ]
then
	ffmpeg -s 1920x1080 -pix_fmt yuv420p -f rawvideo -i images/jelly-1920-1080.YU12 -pix_fmt nv12 -f rawvideo images/jelly-1920-1080.NV12
fi



function codec {

   case "$5" in
    YU12) ffp=yuv420p ;;
    RGB3) ffp=rgb24 ;;
    NV12) ffp=nv12 ;;
   esac

   echo "ffp=$ffp"

   v4l2-ctl -d0 --set-selection-output target=crop,width=$1,height=$2   -x width=$3,height=$4,pixelformat=$5 --stream-mmap --stream-out-mmap --stream-to jelly_$1-$2-$5.fwht --stream-from images/jelly-$3-$4.$5 --all

   v4l2-ctl -d1 -x width=$1,height=$2 -v width=$1,height=$2,pixelformat=$5 --stream-mmap --stream-out-mmap --stream-from jelly_$1-$2-$5.fwht --stream-to out-$1-$2.$5 --all

   ffplay -v info -f rawvideo -pixel_format $ffp -video_size $1x$2  out-$1-$2.$5

}

codec 1440 900 1920 1080 NV12

codec 902 902 902 902 RGB3

codec 642 642 1920 1080 RGB3
codec 642 642 1920 1080 YU12

codec 1440 900  1920 1080 RGB3
codec 1440 900  1920 1080 YU12

codec 1920 1080 1920 1080 RGB3
codec 1920 1080 1920 1080 YU12


