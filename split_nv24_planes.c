#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int get_chr(int fd1, int ufd, int vfd, unsigned char* f1, unsigned int sz,
	    unsigned char* h1, unsigned char* h2)
{
	int ret = read(fd1,f1,sz);
	int ih1 = 0;
	int ih2 = 0;

	if(ret != sz) {
		printf("reading frame %d\n", ret);
		return ret;
	}
	for (unsigned int i = 0; i < sz; i++) {
		if (i%50000 == 0)
			printf("%u out of %u\n", i, sz);
		if (i%2 == 0)
			h1[ih1++] = f1[i];
		else
			h2[ih2++] = f1[i];
	}
	ret = write(ufd,h1,sz/2);
	ret = write(vfd,h2,sz/2);
	return ret;
}

int get_var(int fd1, int to, unsigned char* f1, unsigned int sz){
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

	unsigned int frame_size = w*h*2;
	unsigned char* frame1 = malloc(frame_size);
	unsigned char* helper1 = malloc(w*h);
	unsigned char* helper2 = malloc(w*h);

	int x = 0;
	for(;;) {
		printf("%d\n",x++);
		int var = get_var(fd1, yfd, frame1, w*h);
		if(var <= 0)
			return var;
		var = get_chr(fd1, ufd, vfd, frame1, w*h*2, helper1, helper2);
		if(var <= 0)
			return var;
	}
	return 0;
}
