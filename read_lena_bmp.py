# from __future__ import print_function
import os, sys
from PIL import Image

#lena_gray.[bmp|raw] are from
#http://eeweb.poly.edu/~yao/EL5123/SampleData.html
im = Image.open("lena_gray.bmp")
print(im)
print(im.getbands())
data = im.getdata()
print(data)
print(im.info)
w,h = im.size
pixels = list(im.getdata())
pixels = [pixels[i * w:(i + 1) * w] for i in range(h)]
print(pixels[0])#show the values of the pixels in the first raw
#show the values of the pixels in the last raw as hex
print(list(map(hex,(pixels[-1]))))
