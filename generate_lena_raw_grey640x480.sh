if [ ! -f lena_gray.bmp ]
then
	wget http://eeweb.poly.edu/~yao/EL5123/image/lena_gray.bmp
fi
convert lena_gray.bmp -resize 640x480! lena_grey640_480.bmp
#convert -colors 256 lena_grey640_480.bmp lena_grey640_480g.bmp
convert  lena_grey640_480.bmp lena_grey640_480.ppm
python3 ppm_to_raw_grey.py lena_grey640_480.ppm
rm lena_grey640_480.bmp lena_grey640_480.ppm
