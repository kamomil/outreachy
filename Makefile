CFLAGS=-Wall

target = v4l2-decode v4l2-encode v4l2-encode_grey v4l2-encode1 v4l2-decode_grey v4l2-encode640x480_grey v4l2-decode640x480_grey v4l2-encode-new v4l2-decode-new
all: $(target)

clean:
	rm -f $(target)

.PHONY: clean
