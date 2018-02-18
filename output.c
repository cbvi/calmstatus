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
void pad(int i) { printf("%%{O%i}", i); }

void
do_output(procinfo_t *info)
{
	int level, mute;
	uint32_t curdesktop, actlist[10];
	char title[MAX_TITLE_LENGTH];

	if (pthread_mutex_trylock(&mut) != 0)
		return;

	left();

	curdesktop = xstuff_currentdesktop(info->xstuff);
	xstuff_activeworkspaces(info->xstuff, actlist);
	xstuff_windowtitle(info->xstuff, title, sizeof(title));

	print_workspaces(actlist, curdesktop);

	printf("%s ", title);

	right();
	printf(" ");

	/* volume_print_volume(info->soundinfo); */
	level = volume_level(info->volume);
	mute = volume_mute(info->volume);
	printf("%i%%%s", level, mute ? " (muted)" : "");

	pad(100);

	printf(" ");
	print_datetime();
	printf("%s", "  ");

	printf("\n");
	fflush(stdout);

	pthread_mutex_unlock(&mut);
}

int
output_main(procinfo_t *info)
{
	enum priv_cmd cmd;
	int running = 1;

	while (running) {
		cmd = priv_get_cmd(info->output);

		switch (cmd) {
		case CMD_OUTPUT_DO:
			do_output(info);
			break;
		default:
			running = 0;
			break;
		}
	}
	warnx("output_main: invalid cmd");
	destroy_procinfo(info);

	return 1;
}
