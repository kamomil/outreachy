CFLAGS=-Wall

target = v4l2-decode v4l2-encode

all: $(target)

clean:
	rm -f $(target)

.PHONY: clean
