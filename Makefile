CC=clang
XF=-I/usr/X11R6/include -L/usr/X11R6/lib -lxcb -lxcb-util -lpthread
#XF=-I/usr/X11R6/include -L/usr/X11R6/lib -lX11

all:
	#$(CC) -o zztime -ggdb -std=c89 -Wall -Wextra time.c
	#$(CC) -O3 -o xxwinf $(XF) -ggdb -Wall -Wextra winf.c
	$(CC) -o calmstatus $(XF) -ggdb -Wall -Wextra xstuff.c output.c
	$(CC) -o a.out -lm -ggdb -Wall -Wextra vol.c
