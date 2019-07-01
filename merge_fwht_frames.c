#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

typedef unsigned int __be32;
typedef unsigned int u32;

#define MAX_HEIGHT		2160U
#define MAX_WIDTH		4096U

struct fwht_cframe_hdr {
        u32 magic1;
        u32 magic2;
        __be32 version;
        __be32 width, height;
        __be32 flags;
        __be32 colorspace;
        __be32 xfer_func;
        __be32 ycbcr_enc;
        __be32 quantization;
        __be32 size;
};

/*
 * Assume that the fwht stream is valid and that each
 * frame starts right after the previous one.
 */
int read_fwht_frame(unsigned char *buf, int fd)
{
	struct fwht_cframe_hdr *h = (struct fwht_cframe_hdr *)buf;

	ssize_t sz1 = read(fd, buf, sizeof(struct fwht_cframe_hdr));
	if (!sz1)
		return 0;

	if (sz1 < sizeof(struct fwht_cframe_hdr)) {
		printf("error 1\n");
		return -1;
	}
	printf("frame is %u\n", ntohl(h->size));
	ssize_t sz2 = read(fd, buf + sz1, ntohl(h->size));
	if (sz2 < ntohl(h->size)) {
		printf("error 2\n");
		return -1;
	}
	return sz1 + sz2;
}

int main(int argc, char **argv) {
	int ret;
	char frame_name[256];
	int i = 0;

	if(argc < 5) {
		printf("usage: %s <fwht file 1> <fwht file 2> <fwht to file> <max width> <max height>\n", argv[0]);
		_exit(1);
	}

	int fd1 = open(argv[1], O_RDONLY, 0);
	if(fd1 < 1) {
		printf("could not open flie 1\n");
		_exit(1);
	}
	int fd2 = open(argv[2], O_RDONLY, 0);
	if(fd2 < 1) {
		printf("could not open flie 2\n");
		_exit(1);
	}
	int yfd = open(argv[3], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (yfd <= 0) {
		printf("could not open flie %s\n", frame_name);
		_exit(1);
	}
	unsigned int w = atoi(argv[4]);
	unsigned int h = atoi(argv[5]);
	unsigned char* buff = malloc(w * h * 3);

	while(1) {
		int s = read_fwht_frame(buff, fd1);
		if (!s)
			break;
		if (s < 0)
			_exit(1);
		write(yfd, buff, s);

		s = read_fwht_frame(buff, fd2);
		if (!s)
			break;
		if (s < 0)
			_exit(1);
		write(yfd, buff, s);
	}
	close(yfd);
	return 0;
}
