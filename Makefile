CFLAGS=-Wall

target = v4l2-decode v4l2-encode v4l2-encode_grey v4l2-encode1

all: $(target)

clean:
	rm -f $(target)

.PHONY: clean
