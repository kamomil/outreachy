#!/bin/bash
set -x

if [ ! -f jellyfish-10-mbps-hd-h264.mkv ]
then
	wget http://jell.yfish.us/media/jellyfish-10-mbps-hd-h264.mkv
fi
if [ ! -d images ]
then
	mkdir images
fi

if [ ! -f images/jelly-1920-1080.YU12 ]
then
	ffmpeg -i jellyfish-10-mbps-hd-h264.mkv -c:v rawvideo -pix_fmt yuv420p -f rawvideo /tmp/tmp
	#reduce the video length by half, by removing the first 225 and last 255 frames.
	tail -c $((3*1920*1080*675/2)) /tmp/tmp | head -c $((3*1920*1080*450/2)) > images/jelly-1920-1080.YU12
	rm /tmp/tmp
fi

if [ ! -f images/jelly-1920-1080.GREY ]
then
ffmpeg -s 1920x1080 -pix_fmt yuv420p -f rawvideo -i images/jelly-1920-1080.YU12 -pix_fmt gray -f rawvideo images/jelly-1920-1080.GREY
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

if [ ! -f images/jelly-1920-1080.BA24 ]
then
	head -c $((1920*1080*3*225)) images/jelly-1920-1080.RGB3 > tmp1
	tail -c $((1920*1080*3*225)) images/jelly-1920-1080.RGB3 > tmp2
	ffmpeg -pix_fmt rgb24 -s 1920x1080 -f rawvideo  -i tmp1 -pix_fmt rgb24 -s 1920x1080 -f rawvideo  -i tmp2 -filter_complex "[1]format=argb,colorchannelmixer=aa=0.5[front];[0][front]overlay=x=(W-w)/2:y=H-h" -pix_fmt argb -s 1920x1080 -f rawvideo  images/jelly-1920-1080.BA24
	rm tmp1 tmp2
fi


function codec {

format=${@:$#}
w=$1
h=$2
cr_w=$1
cr_h=$2
cp_w=$1
cp_h=$2
for var in "${@:3:2}"
do
target=${var%=*}
dim=${var#*=}
t_w=${dim%x*}
t_h=${dim#*x}

if [ $target = "crop" ]; then
cr_w=$t_w
cr_h=$t_h
cp_w=$t_w
cp_h=$t_h
fi
if [ $target = "compose" ]; then
cp_w=$t_w
cp_h=$t_h
fi
done

echo "w=$w h=$h cr_w=$cr_w cr_h=$cr_h cp_w=$cp_w cp_h=$cp_h format=$format"
   case "$format" in

    YU12) ffp=yuv420p ;;
    RGB3) ffp=rgb24 ;;
    NV12) ffp=nv12 ;;
    BA24) ffp=argb ;;
    GREY) ffp=gray ;;
    *)    echo "pixelformat should be one of YU12, RGB3, BA24, GREY, NV12"
          exit 1
          ;;
   esac

   echo "ffp=$ffp"

   v4l2-ctl -d0 --set-selection-output target=crop,width=$cr_w,height=$cr_h   -x width=$w,height=$h,pixelformat=$format --stream-mmap --stream-out-mmap --stream-to jelly_$cr_w-$cr_h-$format.fwht --stream-from images/jelly-$w-$h.$format || { echo 'v4l2-ctl -d0 failed' ; exit 1; }

   v4l2-ctl -d1 --set-selection target=compose,width=$cp_w,height=$cp_h -x width=$cr_w,height=$cr_h -v width=$cp_w,height=$cp_h,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from jelly_$cr_w-$cr_h-$format.fwht --stream-to out-$cp_w-$cp_h.$format || { echo 'v4l2-ctl -d1 failed' ; exit 1; }


   ffplay -v info -f rawvideo -pixel_format $ffp -video_size "${cp_w}x${cp_h}"  out-$cp_w-$cp_h.$format

}

if [ "$#" -eq 3 ]; then
    codec $1 $2 $3

elif [ "$#" -eq 0 ]; then

    codec 1920 1080 crop=640x480 GREY
    codec 1920 1080 crop=860x540 compose=800x500  GREY
    codec 1920 1080 compose=600x600 GREY
    codec 1920 1080 GREY
    codec 1920 1080 BA24
    codec 1920 1080 NV12
    codec 902 902 RGB3

    codec 1920 1080 RGB3
    codec 1920 1080 YU12

else
   echo "usage: [crop-width] [crop-height] [width] [height] [pixelformat]"
   echo "if no args are give then some default tests run"
fi


