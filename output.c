#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <pthread.h>

#include "calmstatus.h"

pthread_mutex_t mut;

void
init_output()
{
	pthread_mutex_init(&mut, NULL);
}

void *
xcalloc(size_t n, size_t sz)
{
	void *p;

	if ((p = calloc(n, sz)) == NULL) {
		warnx("calloc failed allocating %lu * %lu", n, sz);
		abort();
	}

	return p;
}

void left() { printf("%s", "%{l}"); }
void right() { printf("%s", "%{r}"); }

void
do_output(info_t *info)
{
	int level, mute;
	uint32_t curdesktop, actlist[10];
	size_t tmp;

	if (pthread_mutex_trylock(&mut) != 0)
		return;

	left();
	print_workspaces(info->xinfo);
	print_title(info->xinfo);

	curdesktop = xstuff_currentdesktop(info->procinfo->xstuff);
	xstuff_activeworkspaces(info->procinfo->xstuff, actlist);

	for (tmp = 0; tmp < 10; tmp++) {
		printf("%u ", actlist[tmp]);
	}
	printf("\n");

	right();
	printf(" ");

	/* volume_print_volume(info->soundinfo); */
	level = volume_level(info->procinfo->volume);
	mute = volume_mute(info->procinfo->volume);
	printf("%i%s", level, mute ? " (muted)" : "");

	printf(" ");
	print_datetime();
	printf("%s", "  ");

	printf("\n");
	fflush(stdout);

	pthread_mutex_unlock(&mut);
}
