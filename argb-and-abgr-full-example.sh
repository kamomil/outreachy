make

images=( "globe-scene-fish-bowl-pngcrush.png" "AlphaBall.png" "AlphaEdge.png" "black817-480x360-3.5.png" )

i=1

for file in "${images[@]}"
do
         if [ ! -f alpha/$file ]
         then
	         wget http://www.libpng.org/pub/png/img_png/$file -o alpha/$file
         fi
         convert alpha/$file -resize 640x480! $i-rgba.png

         ffmpeg -vcodec png -i $i-rgba.png -vcodec rawvideo -f rawvideo -pix_fmt bgra -y $i-V4L2_PIX_FMT_ABGR32-640x480.raw
         ffmpeg -vcodec png -i $i-rgba.png -vcodec rawvideo -f rawvideo -pix_fmt argb -y $i-V4L2_PIX_FMT_ARGB32-640x480.raw
         rm $i-rgba.png
         i=$((i+1))
done

for i in $(seq 4)
do
    ./v4l2-encode-new /dev/video2 "${i}-V4L2_PIX_FMT_ARGB32-640x480.raw" "${i}-argb.fwht" V4L2_PIX_FMT_ARGB32
    ./v4l2-decode-new /dev/video3 "${i}-argb.fwht" "${i}-argb.raw" V4L2_PIX_FMT_ARGB32
done

for i in $(seq 4); do python3 plot_raw_argb_img.py ${i}-argb.raw 640 480; done

for i in $(seq 4)
do
    ./v4l2-encode-new /dev/video2 "${i}-V4L2_PIX_FMT_ABGR32-640x480.raw" "${i}-bgra.fwht" V4L2_PIX_FMT_ABGR32
    ./v4l2-decode-new /dev/video3 "${i}-bgra.fwht" "${i}-bgra.raw" V4L2_PIX_FMT_ABGR32
done

for i in $(seq 4); do python3 plot_raw_bgra_img.py ${i}-bgra.raw 640 480; done
