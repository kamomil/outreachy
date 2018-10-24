/* V4L2 video encoder
   Copyright (C) 2018 Helen Koike <helen.koike@collabora.com>,
                      Dafna Hirschfeld <dafna3@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <unistd.h>

#ifndef V4L2_PIX_FMT_FWHT
#define V4L2_PIX_FMT_FWHT     v4l2_fourcc('F', 'W', 'H', 'T') /* Fast Walsh Hadamard Transform (vicodec) */
#endif


#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define WIDTH  640
#define HEIGHT 480

struct buffer {
        void   *start;
        size_t length;
};

struct buffer *prepare_buffers(int fd, int type)
{
        struct v4l2_requestbuffers      req;
        struct buffer                   *buffer;
        struct v4l2_buffer              buf;

        CLEAR(req);
        req.count = 1;
        req.type = type;
        req.memory = V4L2_MEMORY_MMAP;
        //handled by v4l_reqbufs
        //Initiate Memory Mapping, User Pointer I/O or DMA buffer I/O
        int ret = ioctl(fd, VIDIOC_REQBUFS, &req);
        if(ret){
          perror("ioctl VIDIOC_REQBUFS");
          exit(EXIT_FAILURE);

        }
        else{
          printf("ioctl VIDIOC_REQBUFS for type %d was ok, ask for %d buffers\n",type, req.count);
        }
        if (req.count != 1) {
                perror("this app requires different number of buffers");
                exit(EXIT_FAILURE);
        }

        /* Request buffers */
        buffer = calloc(req.count, sizeof(*buffer));
        CLEAR(buf);

        buf.type        = type;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = 0;

                //VIDIOC_QUERYBUF - Query the status of a buffer
        ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        if(ret){
          perror("ioctl VIDIOC_QUERYBUF");
          exit(EXIT_FAILURE);
        }
        
        printf("prepare_buffers: got buf length %u\n",buf.length);
        buffer[0].length = buf.length;
        buffer[0].start = mmap(NULL, buf.length,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              fd, buf.m.offset);

        if (MAP_FAILED == buffer[0].start) {
          perror("mmap");
          exit(EXIT_FAILURE);
        }



        /* Queue buffer to the capture */
        if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE){
          CLEAR(buf);
          buf.type = type;
          buf.memory = V4L2_MEMORY_MMAP;
          buf.index = 0;
          ret = ioctl(fd, VIDIOC_QBUF, &buf);
          if(ret){
            perror("ioctl VIDIOC_QBUF");
            exit(EXIT_FAILURE);
          }
        }

        return buffer;
}

void recv_frames(int fd, struct buffer *buffers, char* to_img)
{
        struct v4l2_buffer              buf;
        unsigned int                    i;
        FILE                            *fcap;
        int                             ret;
        printf("recv_frames: start\n");

        for (i = 0; ; i++) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                ret = ioctl(fd, VIDIOC_DQBUF, &buf);
                if(ret){
                  perror("recv_frames: ioctl VIDIOC_DQBUF");
                  exit(EXIT_FAILURE);
                }

                fcap = fopen(to_img, "w");
                if (!fcap) {
                        perror("Cannot open image");
                        exit(EXIT_FAILURE);
                }
                printf("recv_frames: writing %u bytes\n",buf.bytesused);
                fwrite(buffers[buf.index].start, buf.bytesused, 1, fcap);
                fclose(fcap);

                if (buf.flags & V4L2_BUF_FLAG_LAST)
                        break;

                //VIDIOC_QBUF - VIDIOC_DQBUF - Exchange a buffer with the driver
                ret = ioctl(fd, VIDIOC_QBUF, &buf);
                if(ret){
                  perror("recv_frames: ioctl VIDIOC_QBUF");
                  exit(EXIT_FAILURE);
                }

        }
}

void send_frames(int fd, struct buffer *buffer, char* from_img)
{
        struct v4l2_buffer              buf;
        FILE                            *fout;
        long                            numbytes;
        struct v4l2_encoder_cmd         enc;
        int ret;

        fout = fopen(from_img, "r");
        if (!fout) {
          perror("Cannot open image");
          exit(EXIT_FAILURE);
        }
        //move pos to eof
        fseek(fout, 0L, SEEK_END);
        numbytes = ftell(fout);
        fseek(fout, 0L, SEEK_SET);


        CLEAR(buf);
        printf("send_frames: copying %ld bytes from %s\n",numbytes,from_img);
        buf.bytesused = fread(buffer[0].start, 1, numbytes, fout);
        printf("send_frames: copying %ld bytes from %s, first byte is 0x%02x\n",numbytes,from_img,  *((unsigned char*)buffer[0].start));
        fclose(fout);
        printf("send_frames: calling VIDIOC_QBUF ioctl\n");
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = 0;
        ret = ioctl(fd, VIDIOC_QBUF, &buf);
        if(ret){
          perror("send_frames: ioctl VIDIOC_QBUF");
          exit(EXIT_FAILURE);
        }

        CLEAR(enc);
        enc.cmd = V4L2_ENC_CMD_STOP;

	printf("send_frames: calling VIDIOC_ENCODER_CMD ioctl\n");
        ret = ioctl(fd, VIDIOC_ENCODER_CMD, &enc);
        if(ret){
          perror("ioctl VIDIOC_ENCODER_CMD");
          exit(EXIT_FAILURE);

        }
        printf("send_frames: returning\n");

}

/*
  struct v4l2_format {
  __u32	 type;
  union {
  struct v4l2_pix_format		pix;     // V4L2_BUF_TYPE_VIDEO_CAPTURE
  struct v4l2_pix_format_mplane	pix_mp;  // V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
  struct v4l2_window		win;     // V4L2_BUF_TYPE_VIDEO_OVERLAY
  struct v4l2_vbi_format		vbi;     // V4L2_BUF_TYPE_VBI_CAPTURE
  struct v4l2_sliced_vbi_format	sliced;  // V4L2_BUF_TYPE_SLICED_VBI_CAPTURE
  struct v4l2_sdr_format		sdr;     // V4L2_BUF_TYPE_SDR_CAPTURE
  struct v4l2_meta_format		meta;    // V4L2_BUF_TYPE_META_CAPTURE
  __u8	raw_data[200];                   // user-defined
  } fmt;
};


 */

void print_cap(struct v4l2_capability vcap) {

  printf("driver = %s\n",vcap.driver);
  printf("device = %s\n",vcap.card);
  printf("bus_info = %s\n",vcap.bus_info);
  printf("version = %u.%u.%u\n",(vcap.version >> 16) & 0xFF, (vcap.version >> 8) & 0xFF, vcap.version & 0xFF);
//vicodec returns: 0x84208000
//this is V4L2_CAP_VIDEO_M2M_MPLANE|V4L2_CAP_EXT_PIX_FORMAT|V4L2_CAP_STREAMING|V4L2_CAP_DEVICE_CAPS
  printf("capabilities = 0x%08x\n",vcap.capabilities);
  printf("device_caps  = 0x%08x\n",vcap.device_caps);
  

}

unsigned int color_format_str_to_id(char* format){

  printf("got %s\n",format);

  if(!strcmp(format, "V4L2_PIX_FMT_ABGR32")){
    printf("got       V4L2_PIX_FMT_ABGRB32 %s\n",format);
    return V4L2_PIX_FMT_ABGR32;
  }
  if(!strcmp(format, "V4L2_PIX_FMT_ARGB32")) {
    printf("got       V4L2_PIX_FMT_ARGB32 %s\n",format);
    return V4L2_PIX_FMT_ARGB32;
  }
  if(!strcmp(format, "V4L2_PIX_FMT_BGR24"))
    return V4L2_PIX_FMT_BGR24;
  if(!strcmp(format, "V4L2_PIX_FMT_BGR32"))
    return V4L2_PIX_FMT_BGR32;
  if(!strcmp(format, "V4L2_PIX_FMT_GREY"))
    return V4L2_PIX_FMT_GREY;
  if(!strcmp(format, "V4L2_PIX_FMT_HSV24"))
    return V4L2_PIX_FMT_HSV24;
  if(!strcmp(format, "V4L2_PIX_FMT_HSV32"))
    return V4L2_PIX_FMT_HSV32;
  if(!strcmp(format, "V4L2_PIX_FMT_NV12"))
    return V4L2_PIX_FMT_NV12;
  if(!strcmp(format, "V4L2_PIX_FMT_NV16"))
    return V4L2_PIX_FMT_NV16;
  if(!strcmp(format, "V4L2_PIX_FMT_NV21"))
    return V4L2_PIX_FMT_NV21;
  if(!strcmp(format, "V4L2_PIX_FMT_NV24"))
    return V4L2_PIX_FMT_NV24;
  if(!strcmp(format, "V4L2_PIX_FMT_NV42"))
    return V4L2_PIX_FMT_NV42;
  if(!strcmp(format, "V4L2_PIX_FMT_NV61"))
    return V4L2_PIX_FMT_NV61;
  if(!strcmp(format, "V4L2_PIX_FMT_RGB24"))
    return V4L2_PIX_FMT_RGB24;
  if(!strcmp(format, "V4L2_PIX_FMT_RGB32"))
    return V4L2_PIX_FMT_RGB32;
  if(!strcmp(format, "V4L2_PIX_FMT_UYVY"))
    return V4L2_PIX_FMT_UYVY;
  if(!strcmp(format, "V4L2_PIX_FMT_VYUY"))
    return V4L2_PIX_FMT_VYUY;
  if(!strcmp(format, "V4L2_PIX_FMT_XBGR32"))
    return V4L2_PIX_FMT_XBGR32;
  if(!strcmp(format, "V4L2_PIX_FMT_XRGB32"))
    return V4L2_PIX_FMT_XRGB32;
  if(!strcmp(format, "V4L2_PIX_FMT_YUV420"))
    return V4L2_PIX_FMT_YUV420;
  if(!strcmp(format, "V4L2_PIX_FMT_YUV422P"))
    return V4L2_PIX_FMT_YUV422P;
  if(!strcmp(format, "V4L2_PIX_FMT_YUYV"))
    return V4L2_PIX_FMT_YUYV;
  if(!strcmp(format, "V4L2_PIX_FMT_YVU420"))
    return V4L2_PIX_FMT_YVU420;
  if(!strcmp(format, "V4L2_PIX_FMT_YVYU"))
    return V4L2_PIX_FMT_YVYU;
  return -1;
}

int main(int argc, char **argv)
{
        struct v4l2_format              fmt;
        enum v4l2_buf_type              type;
        int                             fd = -1;
        char                            *dev_name = NULL;
        struct buffer                   *buffers_cap;
        struct buffer                   *buffers_out;
        int ret = 0;

        if (argc < 5) {
                printf("usage: %s dev-file raw-img fwht-output-img FORMAT\n", argv[0]);
                printf("'dev_file' is the name of the vicodec device file, e.g. /dev/video1\n");
                printf("'raw-img': the name of the raw img, assumed to be 640x480\n");
                printf("'fwht-output-img' the name of the output image\n");
                printf("'FORMAT': the format of raw-img, e.g. V4L2_PIX_FMT_ARGB32\n");

                exit(EXIT_FAILURE);
        }
        dev_name = argv[1];
        char* raw_img = argv[2];
        char* fwht_img = argv[3];
        unsigned int format = color_format_str_to_id(argv[4]);

        if(format == -1){
          perror("bad color format");
          exit(EXIT_FAILURE);
        }


        fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                perror("Cannot open device");
                exit(EXIT_FAILURE);
        }

        struct v4l2_capability vicodec_cap;
        CLEAR(vicodec_cap);
        ret = ioctl(fd, VIDIOC_QUERYCAP, &vicodec_cap);
        if(ret){
          perror("ioctl - try other /dev/video* file");
          return -1;
        }
        print_cap(vicodec_cap);
        /* Set formats in capture and output */
        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        fmt.fmt.pix.width       = WIDTH;
        fmt.fmt.pix.height      = HEIGHT;
        fmt.fmt.pix.pixelformat = format;
        fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_RAW;
        //handled by v4l_s_fmt in v4l2-ioctl.c
        ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
        if(ret){
          perror("ioctl - try other /dev/video* file");
          return -1;
        }

        if (fmt.fmt.pix.pixelformat != format) {
          printf("Driver didn't accept the given format. Can't proceed.\n");
          printf("fmt.fmt.pix.pixelformat: %d\n",fmt.fmt.pix.pixelformat);
          printf("given format = %d\n",format);
          exit(EXIT_FAILURE);
        }
        if ((fmt.fmt.pix.width != WIDTH) || (fmt.fmt.pix.height != HEIGHT))
          printf("Warning: driver is sending image at %dx%d\n",
                 fmt.fmt.pix.width, fmt.fmt.pix.height);

        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_FWHT;
        ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
        if(ret){
          perror("ioctl VIDIOC_S_FMT");
          exit(EXIT_FAILURE);
        }

        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_FWHT) {
                printf("Driver didn't accept FWHT format. Can't proceed.\n");
                printf("fmt.fmt.pix.pixelformat: %d\n",fmt.fmt.pix.pixelformat);
                printf("V4L2_PIX_FMT_FWHT = %d\n",V4L2_PIX_FMT_FWHT);
                exit(EXIT_FAILURE);
        }

        /* Allocate buffers in capture and output */
        buffers_out = prepare_buffers(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);//our output are the ppm files
        buffers_cap = prepare_buffers(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE);

        /* Start streaming */
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMON, &type);
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(fd, VIDIOC_STREAMON, &type);

        /* Send the images to be encoded */
        send_frames(fd, buffers_out, raw_img);
        recv_frames(fd, buffers_cap, fwht_img);

        /* Complete */
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
        if(ret){
          perror("ioctl VIDIOC_STREAMOFF CAPTURE");
          exit(EXIT_FAILURE);
        }

        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        if(ret){
          perror("ioctl VIDIOC_STREAMOFF OUTPUT");
          exit(EXIT_FAILURE);
        }

        /* Clean up*/
        munmap(buffers_cap[0].start, buffers_cap[0].length);
        munmap(buffers_out[0].start, buffers_out[0].length);
        free(buffers_cap);
        free(buffers_out);

        close(fd);

        return 0;
}
