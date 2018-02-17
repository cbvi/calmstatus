#include <sys/socket.h>

#include <err.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "calmstatus.h"
#include "priv.h"

static info_t *
get_info()
{
	info_t *info;

	info = xcalloc(1, sizeof(info_t));

	info->xinfo = get_xinfo();
	info->soundinfo = volume_get_soundinfo();

	return info;
}

static procinfo_t **
get_procinfo()
{
	procinfo_t **info;
	int volsock[2], xsock[2];
	int i;

	info = xcalloc(2, sizeof(procinfo_t *));
	priv_socketpair(volsock);
	priv_socketpair(xsock);

	for (i = 0; i < 2; i++) {
		info[i] = xcalloc(1, sizeof(procinfo_t));
		info[i]->volume = xcalloc(1, sizeof(struct imsgbuf));
		info[i]->xstuff = xcalloc(1, sizeof(struct imsgbuf));
		imsg_init(info[i]->volume, volsock[i]);
		imsg_init(info[i]->xstuff, xsock[i]);
	}

	return info;
}

void
destroy_procinfo(procinfo_t *info)
{
	imsg_clear(info->volume);
	imsg_clear(info->xstuff);

	free(info);
}

void
destroy_info(info_t *info)
{
	destroy_xinfo(info->xinfo);
	volume_destroy_soundinfo(info->soundinfo);
	free(info);
}

int
main()
{
	info_t *info;
	procinfo_t **procinfo;
	pthread_t  dth;

	procinfo = get_procinfo();

	if (fork() == 0) {
		destroy_procinfo(procinfo[0]);
		return volume_main(procinfo[1]);
	}

	if (fork() == 0) {
		destroy_procinfo(procinfo[0]);
		return xstuff_main(procinfo[1]);
	}

	destroy_procinfo(procinfo[1]);

	info = get_info();
	info->procinfo = procinfo[0];

	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	init_output();

	pthread_create(&dth, NULL, watch_for_datetime_changes, info);

	for (;;) {
		do_output(info);
		sleep(2);
	}

	return 0;
}
