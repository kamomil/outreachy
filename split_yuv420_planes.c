#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int get_var(int fd1, int to, unsigned char* f1, unsigned int sz, int d){
	int ret = read(fd1,f1,sz);
	if(ret != sz) {
		printf("reading frame %d\n", ret);
		return ret;
	}
	ret = write(to,f1,sz);
	if(ret != sz) {
		printf("reading frame %d\n", ret);
		return ret;
	}
	return ret;

}

int main(int argc, char **argv) {
	int ret;
	if(argc < 7) {
		printf("uage: <video file name> <width> <height> <y file> <u file> <v file>\n");
		_exit(1);
	}
	int fd1 = open(argv[1], O_RDONLY, 0);
	if(fd1<1) {
		printf("could not open flie\n");
		_exit(1);
	}
	unsigned int w = atoi(argv[2]);
	unsigned int h = atoi(argv[3]);


	int yfd = open(argv[4], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	int ufd = open(argv[5], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	int vfd = open(argv[6], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

	unsigned int frame_size = w*h;
	unsigned char* frame1 = malloc(frame_size);

	for(;;) {
		int var = get_var(fd1, yfd, frame1, w*h,0);
		if(var <= 0)
			return var;
		var = get_var(fd1, ufd, frame1, w*h/4,0);
		if(var <= 0)
			return var;
		var = get_var(fd1, vfd, frame1,  w*h/4,0);
		if(var <= 0)
			return var;
	}
	return 0;
}
