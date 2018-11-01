## encoding-decoding greyscale

* to media tree , master, commit 8caec72e8cbff65afa38928197bea5a393b67975

* I frist added the patches of debug prints: 0002-dafna-prints.patch

* So, `git am 0001-dafna-prints.patch`

* Then, a silly patch to support greyscale: `git am 0003-greyscale-stupid.patch`

* instead of the two above, can add only the grey patch without prints: `git am 0001-Basic-support-only-to-greyscale.patch` 

* compile and install the kernel and modules

* `sudo modprobe vicodec`

* generate raw greyscale 640x480 image called "lena_grey640_480.ppm.raw" `./generate_lena_raw_grey640x480.sh`

* to encode the image: `./v4l2-encode640x480_grey <encoder dev file>`

* to decode back to the original: `./v4l2-decode640x480_grey <decoder dev file>`

* to plot a raw decoded greyscale image: `python3 plot_raw_grey_img.py <img> <width> <height>`

* for example `python3 plot_raw_grey_img.py lena_grey.raw 640 480`


debugging with printing:

```
sudo su
echo 8 > /proc/sys/kernel/printk
```

for dynamic debug , as root:

```
echo "file codec-fwht.c line 1- +p" > /sys/kernel/debug/dynamic_debug/control
```
* `v4l2-decode.c` is from https://gitlab.collabora.com/koike/v4l2-codec.git

* constantly keep dmesg logs: `tail -f /var/log/kern.log`

* `0001-dafna-prints.patch` debug prints patch


## Video4linux2


I encountered problems compiling the v4l-utils program.
I use ubuntu 18.04, I tried to compile with qt4 but I got a compilation error:

```
qvidcap.cpp:14:10: fatal error: QtMath: No such file or directory
 #include <QtMath>
```

I think that QtMath is available only on qt5

So I installed qt5 with `sudo apt install qtbase5-dev` and configured and compiled again (after make clean),
This time I got compilation error in  moc_qv4l2.cpp, starting with the error:

```
moc_qv4l2.cpp:13:2: error: #error "This file was generated using the moc from 4.8.7. It"
 #error "This file was generated using the moc from 4.8.7. It"
  ^~~~~
```

This cpp file is generated with `moc`  and  `/usr/bin/moc` is a symlink to `/usr/bin/qtchooser` 

`qtchooser` use a conf file to decide which qt to use, apparently the conf file 
`/usr/lib/x86_64-linux-gnu/qt-default/qtchooser/default.conf`
is a symlink to `/usr/share/qtchooser/qt4-x86_64-linux-gnu.conf`,
So I just changed it:

```
sudo ln -s -T /usr/share/qtchooser/qt5-x86_64-linux-gnu.conf /usr/lib/x86_64-linux-gnu/qt-default/qtchooser/default.conf -f
```

==============
Uvcvideo is the module that creates the /dev/video0  /dev/video1, so removing it also removes those files
But it is needed to remove it in order to reinstall videodev since uvcvideo depends on it


### Add support for a new pixel format to vicodec

```
struct v4l2_fwht_pixfmt_info {
        u32 id;
        //used to calc `pix->bytesperline = q_data->width * info->bytesperline_mult;`
        unsigned int bytesperline_mult;
        //used to calculate the `pix->sizeimage` in the function `vidioc_try_fmt`, pix is pixel format from the uapi
        //also set `ctx->q_data[V4L2_M2M_SRC].sizeimage` in `vicodec_open`
        unsigned int sizeimage_mult;
        unsigned int sizeimage_div;
        
        unsigned int luma_step; //param `input_step` for the `encode_plane` func in ./codec-fwht.c
        unsigned int chroma_step; ///param `input_step` for the `encode_plane` func in ./codec-fwht.c
        /* Chroma plane subsampling */
        unsigned int width_div;
        unsigned int height_div;
};
```

From the uapi:

```
/*
 *	V I D E O   I M A G E   F O R M A T
 */
struct v4l2_pix_format {
	__u32			width;
	__u32			height;
	__u32			pixelformat;
	__u32			field;		/* enum v4l2_field */
	__u32			bytesperline;	/* for padding, zero if unused */
	__u32			sizeimage;
	__u32			colorspace;	/* enum v4l2_colorspace */
	__u32			priv;		/* private data, depends on pixelformat */
	__u32			flags;		/* format flags (V4L2_PIX_FMT_FLAG_*) */
	union {
		/* enum v4l2_ycbcr_encoding */
		__u32			ycbcr_enc;
		/* enum v4l2_hsv_encoding */
		__u32			hsv_enc;
	};
	__u32			quantization;	/* enum v4l2_quantization */
	__u32			xfer_func;	/* enum v4l2_xfer_func */
};
```
and in vicodec-core.c:
This struct does not seem to be used
```
struct pixfmt_info {
        u32 id;
        unsigned int bytesperline_mult;
        unsigned int sizeimage_mult;
        unsigned int sizeimage_div;
        unsigned int luma_step;
        unsigned int chroma_step;
        /* Chroma plane subsampling */
        unsigned int width_div;
        unsigned int height_div;
};

```
`perl scripts/checkpatch.pl --types BRACES` shows `if` statements with oneline that has `{`
a multilne one-line will not be detected.
Also `C99_COMMENTS` type suppose to detect `//` comments but does not work


