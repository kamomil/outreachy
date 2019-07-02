#!/bin/bash
set -x

if [ "$#" -ne 5 ]; then
	echo "usage: $0 <format> <width 1> <height 1> <width 2> -h2 <height 2>"
	echo "format should be one of YU12 RGB3 GREY 422P"
	exit 0
fi

format=$1
w1=$2
h1=$3
w2=$4
h2=$5
wmax=$(( w1 > w2 ? w1 : w2 ))
hmax=$(( h1 > h2 ? h1 : h2 ))


declare -A v4l_2_ffmpeg_fmt
v4l_2_ffmpeg_fmt=( ["422P"]="yuv422p" ["YU12"]="yuv420p" ["RGB3"]="rgb24" ["NV12"]="nv12" ["BA24"]="argb" ["GREY"]="gray" ["YUYV"]="yuyv422")
declare -A v4l_2_mul
v4l_2_mul=( ["422P"]="2" ["YU12"]="3" ["RGB3"]="3" ["NV12"]="3" ["BA24"]="4" ["GREY"]="1" ["YUYV"]="2")
declare -A v4l_2_div
v4l_2_div=( ["422P"]=1 ["YU12"]=2 ["RGB3"]=1 ["NV12"]=2 ["BA24"]=1 ["GREY"]=1 ["YUYV"]=1)

function initiate_images_dir {
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
}

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

initiate_images_dir
generate_video $format $w1 $h1
generate_video $format $w2 $h2

v4l2-ctl -d0 --set-ctrl video_gop_size=1 --set-selection-output target=crop,width=$w1,height=$h1 -x width=$w1,height=$h1,pixelformat=$format --stream-mmap --stream-out-mmap --stream-to jelly_$w1-$h1-$format.fwht --stream-from images/jelly-$w1-$h1.$format

v4l2-ctl -d0 --set-ctrl video_gop_size=1 --set-selection-output target=crop,width=$w2,height=$h2 -x width=$w2,height=$h2,pixelformat=$format --stream-mmap --stream-out-mmap --stream-to jelly_$w2-$h2-$format.fwht --stream-from images/jelly-$w2-$h2.$format

gcc merge_fwht_frames.c -o merge_fwht_frames

./merge_fwht_frames jelly_$w1-$h1-$format.fwht jelly_$w2-$h2-$format.fwht merged-dim.fwht $wmax $hmax

v4l2-ctl -d2 --set-ctrl video_gop_size=1 -x width=$wmax,height=$hmax -v width=$wmax,height=$hmax,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from merged-dim.fwht --stream-to out-$wmax-$hmax.$format

size=$(stat --printf="%s" out-$wmax-$hmax.$format)

frm1_sz=$(($w1 * $h1 * ${v4l_2_mul[$format]} / ${v4l_2_div[$format]}))
frm2_sz=$(($w2 * $h2 * ${v4l_2_mul[$format]} / ${v4l_2_div[$format]}))

ex_size1=$(($frm1_sz * 450))
ex_size2=$(($frm2_sz * 450))

if [ $(($ex_size1 + $ex_size2)) != $size ]; then

        echo "worng size"
        echo "actual   size = $size"
        exit 1
fi

double_frame=$(($frm1_sz + $frm2_sz))

i=0
while [[ $i -le 450 ]]
do
	dd if=out-$wmax-$hmax.$format obs=$double_frame ibs=$double_frame skip=$i count=1 >> tmp
	head -c $frm1_sz tmp >> out-mrg-$w1-$h1.$format
	tail -c $frm2_sz tmp >> out-mrg-$w2-$h2.$format
	rm tmp
	i=$(($i + 1))
done

ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w1}x${h1}"  out-mrg-$w1-$h1.$format
ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w2}x${h2}"  out-mrg-$w2-$h2.$format

