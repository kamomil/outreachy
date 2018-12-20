#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

void valid_read(int fd, unsigned char *buf, unsigned int sz) {
	//printf("valid_read: called with %u\n",sz);
	int ret = read(fd,buf,sz);
	if(ret != sz) {
		printf("error reading frame\n");
		_exit(1);
	}

}
int main(int argc, char **argv) {
	if(argc < 8) {
		printf("uage: <video file name> <width> <height> <plane [l|r|b]> <frame_num> <row> <column>\n");
		printf("frame, row and column numbers start at 0\n");
		_exit(1);
	}
	int fd = open(argv[1], O_RDONLY, 0);
	if(fd<1) {
		printf("could not open flie\n");
		_exit(1);
	}
	unsigned int w = atoi(argv[2]);
	unsigned int h = atoi(argv[3]);

	char plane = argv[4][0];

	char frame_num = atoi(argv[5]);
	unsigned int r = atoi(argv[6]);
	unsigned int c = atoi(argv[7]);

	unsigned int frame_size = w*h*3/2;
	unsigned char* frame = malloc(frame_size);

	for(int i=0;i<frame_num;i++)
		valid_read(fd,frame,frame_size);

	if(plane != 'l')
		valid_read(fd,frame,w*h);

	if(plane == 'r')
		valid_read(fd,frame,w*h/4);

	if(plane != 'l') {
		w /= 2;
		h /= 2;
	}

	for(int i = 0;i<r; i++)
		valid_read(fd,frame,w);

	valid_read(fd, frame, c+1);

	printf("the value in plane '%c', frame %u, raw %u col %u is %u (0x%02x)\n",plane, frame_num, r,c, frame[c], frame[c]);

	return 0;
}
