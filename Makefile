CFLAGS=-Wall

target = v4l2-encode-getchar v4l2-encode-buggy  v4l2-decode-buggy v4l2-decode-getchar v4l2-decode v4l2-encode v4l2-encode640x480_grey v4l2-decode640x480_grey v4l2-encode-new v4l2-decode-new
all: $(target)

clean:
	rm -f $(target)
	rm -f *.raw *.fwht

.PHONY: clean
