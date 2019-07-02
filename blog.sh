#!/bin/bash

if [ "$#" -lt 5 ]; then
	echo "usage: $0 <format> <width 1> <height 1> <width 2> <height 2> [<decoder>]"
	echo "<format> should be one of YU12 RGB3 GREY 422P YUYV"
	echo "<decoder> should be one of stateful or stateless (default is stateless)"
	echo "usage example:"
	echo "$0 GREY 700 1000 800 900"
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

echo "modprobing vicodec - need sudo permissions for that"
sudo modprobe vicodec -v

i=0
c=0
enc_idx=0
dec_less_idx=0
dec_ful_idx=0

while [[ $i -le 6 ]]
do
	if [ "$(v4l2-ctl -d${i} --all 2>/dev/null | grep stateful-encoder-source -o)" == "stateful-encoder-source" ]; then
		enc_idx=$i
		c=$(($c + 1))
	fi
	if [ "$(v4l2-ctl -d${i} --all 2>/dev/null | grep stateless-decoder-source -o)" == "stateless-decoder-source" ]; then
		dec_less_idx=$i
		c=$(($c + 1))
	fi
	if [ "$(v4l2-ctl -d${i} --all 2>/dev/null | grep stateful-decoder-source -o)" == "stateful-decoder-source" ]; then
		dec_ful_idx=$i
		c=$(($c + 1))
	fi
	i=$(($i + 1))
done

if [[ $c -ne 3 ]]; then
	echo "could not find vicodec's pseudo files"
	exit 1
fi

echo "encoder is exposed in /dev/video$enc_idx"
echo "stateless decoder is exposed in /dev/video$dec_less_idx"
echo "stateful decoder is exposed in /dev/video$dec_ful_idx"

rm *.fwht out-*

dec_idx=$dec_less_idx

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
		echo "generating images/jelly-1920-1080.YU12"
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

	if [ ! -f images/jelly-$w-$h.$v4l_fmt ]
	then
		echo "generating images/jelly-$w-$h.$v4l_fmt with ffmpeg"
		ffmpeg -s 1920x1080 -pix_fmt yuv420p -f rawvideo -i images/jelly-1920-1080.YU12 -filter:v "crop=$w:$h:0:0" -pix_fmt $ffmpeg_fmt -f rawvideo images/jelly-$w-$h.$v4l_fmt
	fi
}

initiate_images_dir
generate_video $format $w1 $h1
generate_video $format $w2 $h2

echo "encoding first file, format=$format ${w1}x${h1}"
v4l2-ctl -d${enc_idx} --set-ctrl video_gop_size=1 --set-selection-output target=crop,width=$w1,height=$h1 -x width=$w1,height=$h1,pixelformat=$format --stream-mmap --stream-out-mmap --stream-to jelly_$w1-$h1-$format.fwht --stream-from images/jelly-$w1-$h1.$format

echo "encoding second file, format=$format ${w2}x${h2}"
v4l2-ctl -d${enc_idx} --set-ctrl video_gop_size=1 --set-selection-output target=crop,width=$w2,height=$h2 -x width=$w2,height=$h2,pixelformat=$format --stream-mmap --stream-out-mmap --stream-to jelly_$w2-$h2-$format.fwht --stream-from images/jelly-$w2-$h2.$format

gcc merge_fwht_frames.c -o merge_fwht_frames

echo "merging the files"
./merge_fwht_frames jelly_$w1-$h1-$format.fwht jelly_$w2-$h2-$format.fwht merged-dim.fwht $wmax $hmax

if [[ "$6" == "stateful" ]]; then
	echo "using stateful decoder"
	dec_idx=$dec_ful_idx
fi


echo "decoding with the stateless decoder into out-$wmax-$hmax.$format"
v4l2-ctl -d${dec_idx} --set-ctrl video_gop_size=1 -x width=$wmax,height=$hmax -v width=$wmax,height=$hmax,pixelformat=$format --stream-mmap --stream-out-mmap --stream-from merged-dim.fwht --stream-to out-$wmax-$hmax.$format

size=$(stat --printf="%s" out-$wmax-$hmax.$format)

frm1_sz=$(($w1 * $h1 * ${v4l_2_mul[$format]} / ${v4l_2_div[$format]}))
frm2_sz=$(($w2 * $h2 * ${v4l_2_mul[$format]} / ${v4l_2_div[$format]}))

ex_size1=$(($frm1_sz * 450))
ex_size2=$(($frm2_sz * 450))

if [ $(($ex_size1 + $ex_size2)) != $size ]; then

	echo "wrong size"
	echo "actual size = $size"
	exit 1
fi

double_frame=$(($frm1_sz + $frm2_sz))

echo "splitting back the file..."
i=0
while [[ $i -le 450 ]]
do
	dd if=out-$wmax-$hmax.$format obs=$double_frame ibs=$double_frame skip=$i count=1 status=none >> tmp
	head -c $frm1_sz tmp >> out-mrg-$w1-$h1.$format
	tail -c $frm2_sz tmp >> out-mrg-$w2-$h2.$format
	rm tmp
	i=$(($i + 1))
done

ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w1}x${h1}"  out-mrg-$w1-$h1.$format
ffplay -loglevel warning -v info -f rawvideo -pixel_format "${v4l_2_ffmpeg_fmt[$format]}" -video_size "${w2}x${h2}"  out-mrg-$w2-$h2.$format

