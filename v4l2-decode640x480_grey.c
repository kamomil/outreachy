/* V4L2 video decoder
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
          printf("ioctl VIDIOC_REQBUFS was ok, ask for %d buffers\n",req.count);
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

void recv_frames(int fd, struct buffer *buffers)
{
        struct v4l2_buffer              buf;
        unsigned int                    i;
        FILE                            *fcap;
        int                             ret;
        for (i = 0; ; i++) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                ret = ioctl(fd, VIDIOC_DQBUF, &buf);
                if(ret){
                  perror("recv_frames: ioctl DQBUF");
                  exit(EXIT_FAILURE);
                }

                fcap = fopen("lena_grey.raw", "w");
                if (!fcap) {
                        perror("Cannot open image");
                        exit(EXIT_FAILURE);
                }
                //fprintf(fcap, "P6\n%d %d 255\n", WIDTH, HEIGHT);
                fwrite(buffers[buf.index].start, buf.bytesused, 1, fcap);
                fclose(fcap);

                if (buf.flags & V4L2_BUF_FLAG_LAST)
                        break;

                //VIDIOC_QBUF - VIDIOC_DQBUF - Exchange a buffer with the driver
                int ret = ioctl(fd, VIDIOC_QBUF, &buf);
                if(ret){
                  perror("recv_frames: ioctl VIDIOC_QBUF");
                  exit(EXIT_FAILURE);
                }

        }
}

void send_frames(int fd, struct buffer *buffer)
{
        struct v4l2_buffer              buf;
        char                            *out_name;
        FILE                            *fout;
        long                            numbytes;
        struct v4l2_encoder_cmd         enc;
        int ret;

        out_name = "lena_grey.wfht";
        fout = fopen(out_name, "r");
        if (!fout) {
          perror("Cannot open image");
          exit(EXIT_FAILURE);
        }
        //move pos to eof
        fseek(fout, 0L, SEEK_END);
        numbytes = ftell(fout);
        fseek(fout, 0L, SEEK_SET);


        CLEAR(buf);
        printf("send_frames: copying %ld bytes from %s\n",numbytes,out_name);
        buf.bytesused = fread(buffer[0].start, 1, numbytes, fout);
        fclose(fout);

        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = 0;
        ret = ioctl(fd, VIDIOC_QBUF, &buf);
        if(ret){
          perror("send_frames: ioctl VIDIOC_QBUF");
          exit(EXIT_FAILURE);
        }

        CLEAR(enc);
        enc.cmd = V4L2_DEC_CMD_STOP;
        ret = ioctl(fd, VIDIOC_DECODER_CMD, &enc);
        if(ret){
          perror("ioctl VIDIOC_DECODER_CMD");
          exit(EXIT_FAILURE);

        }

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

        if (argc < 2) {
                printf("usage: %s dev_file\n", argv[0]);
                printf("'dev_file' is the name of the vicodec device file, e.g. /dev/video1\n");
                exit(EXIT_FAILURE);
        }
        dev_name = argv[1];


        fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
                perror("Cannot open device");
                exit(EXIT_FAILURE);
        }

        /* Set formats in capture and output */
        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_FWHT;
        //handled by v4l_s_fmt in v4l2-ioctl.c
        ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
        if(ret){
          perror("ioctl - try other /dev/video* file");
          return -1;
        }
        else{
          printf("ioctl VIDIOC_S_FMT was ok\n");
        }
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_FWHT) {
          printf("Driver didn't accept FWHT format. Can't proceed.\n");
          printf("fmt.fmt.pix.pixelformat: %d\n",fmt.fmt.pix.pixelformat);
          printf("V4L2_BUF_TYPE_VIDEO_OUTPUT = %d\n",V4L2_BUF_TYPE_VIDEO_OUTPUT);
          exit(EXIT_FAILURE);
        }

        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = WIDTH;
        fmt.fmt.pix.height      = HEIGHT;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
        fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_RAW;
        ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
        if(ret){
          perror("ioctl VIDIOC_S_FMT");
          exit(EXIT_FAILURE);
        }

        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_GREY) {
                printf("Driver didn't accept GREY format. Can't proceed.\n");
                printf("fmt.fmt.pix.pixelformat: %d\n",fmt.fmt.pix.pixelformat);
                printf("V4L2_PIX_FMT_GREY = %d\n",V4L2_PIX_FMT_GREY);
                exit(EXIT_FAILURE);
        }
        if ((fmt.fmt.pix.width != WIDTH) || (fmt.fmt.pix.height != HEIGHT))
                printf("Warning: driver is sending image at %dx%d\n",
                        fmt.fmt.pix.width, fmt.fmt.pix.height);

        /* Allocate buffers in capture and output */
        buffers_out = prepare_buffers(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
        buffers_cap = prepare_buffers(fd, V4L2_BUF_TYPE_VIDEO_CAPTURE);

        /* Start streaming */
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMON, &type);
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ioctl(fd, VIDIOC_STREAMON, &type);

        /* Send the images to be encoded */
        send_frames(fd, buffers_out);
        recv_frames(fd, buffers_cap);

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
