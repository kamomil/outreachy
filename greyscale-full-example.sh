if [ "$#" -ne 2 ]; then
    echo "$# usage $0 dev-encode-file dev-decode-file"
    echo "for example: '$0 /dev/video0 /dev/video1'"
    exit 1
fi

make

if [ ! -f lena_gray.bmp ]
then
	wget http://eeweb.poly.edu/~yao/EL5123/image/lena_gray.bmp
fi
convert lena_gray.bmp -resize 640x480! lena_grey640_480.bmp
#convert -colors 256 lena_grey640_480.bmp lena_grey640_480g.bmp
convert  lena_grey640_480.bmp lena_grey640_480.ppm
python3 ppm_to_raw_grey.py lena_grey640_480.ppm
rm lena_grey640_480.bmp lena_grey640_480.ppm

./v4l2-encode640x480_grey /dev/video2
./v4l2-decode640x480_grey /dev/video3

python3 plot_raw_grey_img.py  lena_grey.raw 640 480
