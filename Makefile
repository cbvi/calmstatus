CC=clang
CFLAGS=-ggdb -Wall -Wextra
CFLAGS+=-I/usr/X11R6/include -L/usr/X11R6/lib
CFLAGS+=-lxcb -lxcb-util -lpthread -lm
FILES=xstuff.c output.c datetime.c

all:
	$(CC) -o calmstatus $(CFLAGS) $(FILES)
	$(CC) -o a.out $(CFLAGS) vol.c
