#!/bin/bash
set -x

declare -A v4l_2_ffmpeg_fmt
v4l_2_ffmpeg_fmt=( ["422P"]="yuv422p" ["YU12"]="yuv420p" ["RGB3"]="rgb24" ["NV12"]="nv12" ["BA24"]="argb" ["GREY"]="gray" ["YUYV"]="yuyv422" ["YM24"]="yuv444p")
declare -A v4l_2_mul
v4l_2_mul=( ["422P"]="2" ["YU12"]="3" ["RGB3"]="3" ["NV12"]="3" ["BA24"]="4" ["GREY"]="1" ["YUYV"]="2" ["NV24"]="3")
declare -A v4l_2_div
v4l_2_div=( ["422P"]=1 ["YU12"]=2 ["RGB3"]=1 ["NV12"]=2 ["BA24"]=1 ["GREY"]=1 ["YUYV"]=1 ["NV24"]="1")

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

function generate_nv_video {
	local w=$1
	local h=$2
	if [ ! -f images/jelly-$w-$h.NV24 ]
	then
	sudo modprobe -v vivid
	v4l2-ctl -d3 -v width=$w,height=$h,pixelformat=NV24 --stream-mmap --stream-count=450 --stream-to=images/jelly-$w-$h.NV24 --get-fmt-video || { echo 'vivid failed'; exit 1; }
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
		ffmpeg -s 1920x1080 -pix_fmt yuv420p -f rawvideo -i images/jelly-1920-1080.YU12 -filter:v "crop=$w:$h:0:0" -pix_fmt $ffmpeg_fmt -f rawvideo images/jelly-$w-$h.$v4l_fmt || { echo 'ffmpeg failed' ; exit 1; }

	fi
}

if [ ! -f images/jelly-1920-1080.BA24 ]
then
head -c $((1920*1080*3*225)) images/jelly-1920-1080.RGB3 > tmp1
tail -c $((1920*1080*3*225)) images/jelly-1920-1080.RGB3 > tmp2
ffmpeg -pix_fmt rgb24 -s 1920x1080 -f rawvideo  -i tmp1 -pix_fmt rgb24 -s 1920x1080 -f rawvideo  -i tmp2 -filter_complex "[1]format=argb,colorchannelmixer=aa=0.5[front];[0][front]overlay=x=(W-w)/2:y=H-h" -pix_fmt argb -s 1920x1080 -f rawvideo  images/jelly-1920-1080.BA24
rm tmp1 tmp2
fi

enerate_nv_video 640 480
generate_nv_video 1280 720

W=$1
H=$2

generate_video YUYV $W $H
generate_video 422P $W $H
generate_video GREY $W $H
generate_video YU12 $W $H
generate_video NV12 $W $H
generate_video RGB3 $W $H

