#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "calmstatus.h"

pthread_mutex_t mut;

static volatile sig_atomic_t running = 1;

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

static void
signal_term(int sig)
{
	(void)sig;
	running = 0;
}

static void
terminate_processes(procinfo_t *info)
{
	priv_send_cmd(info->xstuff, CMD_STOP_RIGHT_NOW);
	priv_send_cmd(info->volume, CMD_STOP_RIGHT_NOW);
}

static void left() { printf("%s", "%{l}"); }
static void right() { printf("%s", "%{r}"); }
static void pad(int i) { printf("%%{O%i}", i); }

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

	signal(SIGTERM, signal_term);

	while (running) {
		cmd = priv_get_cmd(info->output);

		switch (cmd) {
		case CMD_OUTPUT_DO:
			do_output(info);
			break;
		case CMD_GOODBYE:
			running = 0;
			break;
		default:
			warnx("output_main: invalid cmd");
			running = 0;
			break;
		}
	}

	terminate_processes(info);
	destroy_procinfo(info);

	return 1;
}
