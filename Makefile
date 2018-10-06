CFLAGS=-Wall

target = v4l2-decode

all: $(target)

clean:
	rm -f $(target)

.PHONY: clean
