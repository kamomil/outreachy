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

#define N_BUFS 5

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define WIDTH  640
#define HEIGHT 480

#define STUPID_LEN 4
struct buffer {
        void   *start;
        size_t length;
};

struct buffer *prepare_buffers(int fd, int type)
{
        struct v4l2_requestbuffers      req;
        struct buffer                   *buffers;
        struct v4l2_buffer              buf;
        unsigned int                    i, n_buffers;

        CLEAR(req);
        req.count = N_BUFS;
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
        if (req.count != N_BUFS) {
                perror("this app requires different number of buffers");
                exit(EXIT_FAILURE);
        }

        /* Request buffers */
        buffers = calloc(req.count, sizeof(*buffers));
        for (n_buffers = 0; n_buffers < N_BUFS; ++n_buffers) {
                CLEAR(buf);

                buf.type        = type;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                //VIDIOC_QUERYBUF - Query the status of a buffer
                ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
                if(ret){
                  perror("ioctl VIDIOC_QUERYBUF");
                  exit(EXIT_FAILURE);

                }
		size_t len = buf.length;
		if(type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
			len = STUPID_LEN;
                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start = mmap(NULL, len,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              fd, buf.m.offset);
                printf("prepare_buffers: mapping %lu, got %p\n",len,buffers[n_buffers].start);

                if (MAP_FAILED == buffers[n_buffers].start) {
                        perror("mmap");
                        exit(EXIT_FAILURE);
                }
        }

        /* Queue buffer to the capture */
        if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
                for (i = 0; i < n_buffers; ++i) {
                        CLEAR(buf);
                        buf.type = type;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;
                        ret = ioctl(fd, VIDIOC_QBUF, &buf);
                        if(ret){
                          perror("ioctl VIDIOC_QBUF");
                          exit(EXIT_FAILURE);

                        }
                }

        return buffers;
}

void recv_frames(int fd, struct buffer *buffers)
{
        struct v4l2_buffer              buf;
        unsigned int                    i;
        char                            out_name[256];
        FILE                            *fcap;

        for (i = 0; ; i++) {
                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                int ret = ioctl(fd, VIDIOC_DQBUF, &buf);
		if(ret){
                  perror("recv_frames: ioctl VIDIOC_DQBUF");
                  exit(EXIT_FAILURE);
                }

		sprintf(out_name, "data/compressed%03d.fwht", i);
                
                fcap = fopen(out_name, "w");
                if (!fcap) {
                        perror("Cannot open image");
                        exit(EXIT_FAILURE);
                }
                //fprintf(fcap, "P6\n%d %d 255\n", WIDTH, HEIGHT);
                printf("recv_frames: writing %u bytes from %p to file\n",buf.bytesused > 4 ? 4: buf.bytesused, buffers[buf.index].start);
                fwrite(buffers[buf.index].start, buf.bytesused > STUPID_LEN ? STUPID_LEN : buf.bytesused, 1, fcap);
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

void send_frames(int fd, struct buffer *buffers)
{
        struct v4l2_buffer              buf;
        unsigned int                    i;
        char                            out_name[256];
        FILE                            *fout;
        long                            numbytes;
        struct v4l2_encoder_cmd         enc;

        for (i = 0; i < N_BUFS; i++) {

                sprintf(out_name, "raw%03d.ppm", i);
                fout = fopen(out_name, "r");
                if (!fout) {
                        perror("Cannot open image");
                        exit(EXIT_FAILURE);
                }
                //move pos to eof
                fseek(fout, 0L, SEEK_END);
                numbytes = ftell(fout) - 15;//15 is the header size
                fseek(fout, 0L, SEEK_SET);

		fread(buffers[i].start, 1, 15, fout);//skip the header
                CLEAR(buf);

                buf.bytesused = fread(buffers[i].start, 1, numbytes, fout);
                printf("send_frames: copying %ld bytes from %s to %p, first byte is 0x%02x\n",numbytes,out_name,buffers[i].start,  *((unsigned char*)buffers[i].start));
                fclose(fout);

                buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                printf("send_frames: before VIDIOC_QBUF\n");
                int ret = ioctl(fd, VIDIOC_QBUF, &buf);
                if(ret){
                  perror("send_frames: ioctl VIDIOC_QBUF");
                  exit(EXIT_FAILURE);
                }
                printf("send_frames: after VIDIOC_QBUF\n");

        }
        CLEAR(enc);
        enc.cmd = V4L2_ENC_CMD_STOP;
        int ret = ioctl(fd, VIDIOC_ENCODER_CMD, &enc);
        if(ret){
          perror("ioctl VIDIOC_ENCODER_CMD");
          exit(EXIT_FAILURE);

        }

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

int main(int argc, char **argv)
{
        struct v4l2_format              fmt;
        enum v4l2_buf_type              type;
        int                             fd = -1;
        unsigned int                    i;
        struct buffer                   *buffers_cap;
        struct buffer                   *buffers_out;
        int ret = 0;

        if (argc < 2) {
                printf("usage: %s dev_file\n", argv[0]);
                printf("'dev_file' is the name of the vicodec device file, e.g. /dev/video1\n");
                exit(EXIT_FAILURE);
        }

        fd = open(argv[1], O_RDWR | O_NONBLOCK, 0);
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
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
        fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_SRGB;
        //handled by v4l_s_fmt in v4l2-ioctl.c

        ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
        if(ret){
          perror("ioctl - try other /dev/video* file");
          return -1;
        }
        
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
          printf("Driver didn't accept RGB24 format. Can't proceed.\n");
          printf("fmt.fmt.pix.pixelformat: %d\n",fmt.fmt.pix.pixelformat);
          printf("V4L2_PIX_FMT_RGB24 = %d\n",V4L2_PIX_FMT_RGB24);
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
//        for (i = 0; i < N_BUFS; ++i) {
//                munmap(buffers_cap[i].start, buffers_cap[i].length);
//                munmap(buffers_out[i].start, buffers_out[i].length);
//        }
        free(buffers_cap);
        free(buffers_out);

        close(fd);

        return 0;
}
