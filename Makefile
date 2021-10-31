CC=clang
CFLAGS=-ggdb -Wall -Wextra
CFLAGS+=-I/usr/X11R6/include -L/usr/X11R6/lib
CFLAGS+=-lxcb -lxcb-util -lpthread -lm -lutil
FILES=xstuff.c output.c datetime.c main.c priv.c

all:
	$(CC) -o calmstatus $(CFLAGS) $(FILES)

release:
	$(CC) -O2 -o calmstatus $(CFLAGS) $(FILES)
