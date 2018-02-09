CC=clang
CFLAGS=-ggdb -Wall -Wextra
CFLAGS+=-I/usr/X11R6/include -L/usr/X11R6/lib -lxcb -lxcb-util -lpthread
FILES=xstuff.c output.c datetime.c

all:
	$(CC) -o calmstatus $(CFLAGS) $(FILES)
	$(CC) -o a.out -lm -ggdb -Wall -Wextra vol.c
