#!/bin/bash
set -x

declare -A v4l_2_ffmpeg_fmt
v4l_2_ffmpeg_fmt=( ["YU12"]="yuv420p" ["RGB3"]="rgb24" ["NV12"]="nv12" ["BA24"]="argb" ["GREY"]="gray" ["YUYV"]="yuyv422")

if [ ! -d images ]
then
	mkdir images
fi

if [ ! -f images/jelly-1920-1080.YU12 ]
then
	if [ ! -f jellyfish-10-mbps-hd-h264.mkv ]
	then
	wget http://jell.yfish.us/media/jellyfish-10-mbps-hd-h264.mkv
	fi
	ffmpeg -i jellyfish-10-mbps-hd-h264.mkv -c:v rawvideo -pix_fmt yuv420p -f rawvideo /tmp/tmp
	#reduce the video length by half, by removing the first 225 and last 255 frames.
	tail -c $((3*1920*1080*675/2)) /tmp/tmp | head -c $((3*1920*1080*450/2)) > images/jelly-1920-1080.YU12
	rm /tmp/tmp
fi

function generate_video {
	local v4l_fmt=$1
	local ffmpeg_fmt=${v4l_2_ffmpeg_fmt[$1]}
	local w=$2
	local h=$3

	echo "$w $h $v4l_fmt $v4l_ffmpeg"
	if [ ! -f images/jelly-$w-$h.$v4l_fmt ]
	then
	ffmpeg -s 1920x1080 -pix_fmt yuv420p -f rawvideo -i images/jelly-1920-1080.YU12 -filter:v "crop=$w:$h:0:0" -pix_fmt $ffmpeg_fmt -f rawvideo images/jelly-$w-$h.$v4l_fmt
	fi
}

generate_video YUYV 802 910
generate_video YUYV 1002 1010
generate_video YU12 802 910
generate_video YU12 1002 1010
generate_video GREY 802 910
generate_video GREY 1002 1010
generate_video RGB3 802 910
generate_video RGB3 1002 1010
generate_video NV12 802 910
generate_video NV12 1002 1010
generate_video BA24 802 910
generate_video BA24 1002 1010

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
   echo "ffp=$ffp"

v4l2-ctl -d0 --set-selection-output target=crop,width=$cr_w,height=$cr_h   -x width=$w,height=$h,pixelformat=$format --stream-mmap --stream-out-mmap --stream-to jelly_$cr_w-$cr_h-$format.fwht --stream-from images/jelly-$w-$h.$format || { echo 'v4l2-ctl -d0 failed' ; exit 1; }

#   v4l2-ctl -d1 -x width=$cr_w,height=$cr_h -v width=$cr_w,height=$cr_h,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from jelly_$cr_w-$cr_h-$format.fwht --stream-to out-$cr_w-$cr_h.$format || { echo 'v4l2-ctl -d1 failed' ; exit 1; }

v4l2-ctl -d1 -x width=$cr_w,height=$cr_h -v width=$cr_w,height=$cr_h,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from jelly_$cr_w-$cr_h-$format.fwht --stream-to out-$cr_w-$cr_h.$format --set-fmt-video-stream pixelformat=$format || { echo 'v4l2-ctl -d1 failed' ; exit 1; }


ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${cp_w}x${cp_h}"  out-$cp_w-$cp_h.$format

}

function decode_res_change {

w1=$1
h1=$2
w2=$3
h2=$4
format=${@:$#}
cat jelly_$w1-$h1-$format.fwht jelly_$w2-$h2-$format.fwht > jelly-$format-$w1-$h1-to-$w2-$h2.fwht
cat jelly_$w2-$h2-$format.fwht jelly_$w1-$h1-$format.fwht > jelly-$format-$w2-$h2-to-$w1-$h1.fwht

v4l2-ctl -d1 -x width=$w1,height=$h1 -v width=$w1,height=$h1,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from jelly-$format-$w1-$h1-to-$w2-$h2.fwht --stream-to out-$w1-$h1-to-$w2-$h2.$format --set-fmt-video-stream pixelformat=$format || { echo 'v4l2-ctl -d1 failed' ; exit 1; }

ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w1}x${h1}"  "out-${w1}-${h1}-to-${w2}-${h2}.$format"
ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w2}x${h2}"  "out-${w1}-${h1}-to-${w2}-${h2}.$format.1"

v4l2-ctl -d1 -x width=$w2,height=$h2 -v width=$w2,height=$h2,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from jelly-$format-$w2-$h2-to-$w1-$h1.fwht --stream-to out-$w2-$h2-to-$w1-$h1.$format --set-fmt-video-stream pixelformat=$format || { echo 'v4l2-ctl -d1 failed' ; exit 1; }

ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w2}x${h2}"  "out-${w2}-${h2}-to-${w1}-${h1}.$format"
ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w1}x${h1}"  "out-${w2}-${h2}-to-${w1}-${h1}.$format.1"

rm jelly-$format-$w1-$h1-to-$w2-$h2.fwht
rm jelly-$format-$w2-$h2-to-$w1-$h1.fwht
}

if [ "$#" -gt 2 ]; then
    codec $@

elif [ "$#" -eq 0 ]; then

#    codec 1920 1080 crop=640x480 GREY
#    codec 1920 1080 crop=860x540 compose=860x540  GREY
#	codec 802 910 crop=802x910 GREY
#	codec 1002 1010 crop=1002x1010 GREY
	decode_res_change 802 910 1002 1010 GREY

	codec 802 910 crop=802x910 YU12
	codec 1002 1010 crop=1002x1010 YU12
	decode_res_change 802 910 1002 1010 YU12

	codec 802 910 crop=802x910 NV12
	codec 1002 1010 crop=1002x1010 NV12

	codec 802 910 crop=802x910 YUYV
	codec 1002 1010 crop=1002x1010 YUYV

	codec 802 910 crop=802x910 RGB3
	codec 1002 1010 crop=1002x1010 RGB3

	codec 802 910 crop=802x910 BA24
	codec 1002 1010 crop=1920x1080 BA24

#    codec 1920 1080 crop=640x480 RGB3
#    codec 1920 1080 crop=860x540 compose=860x540  RGB3
#    codec 1920 1080 compose=1920x1080 RGB3
#    codec 1920 1080 crop=640x480 YU12
#    codec 1920 1080 crop=860x540 compose=860x540  YU12
#    codec 1920 1080 compose=1920x1080 YU12


#    codec 1920 1080 crop=860x540 compose=860x540  GREY
#    codec 1920 1080 compose=640x600 GREY
#     codec 1920 1080 GREY
#    codec 1920 1080 NV12
#    codec 902 902 RGB3

#    codec 1920 1080 RGB3
#    codec 1920 1080 YU12

else
   echo "usage: [crop-width] [crop-height] [width] [height] [pixelformat]"
   echo "if no args are give then some default tests run"
fi


