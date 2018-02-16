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
	int volsock[2];
	int i;

	info = xcalloc(2, sizeof(procinfo_t *));
	priv_socketpair(volsock);

	for (i = 0; i < 2; i++) {
		info[i] = xcalloc(1, sizeof(procinfo_t));
		info[i]->volume = xcalloc(1, sizeof(struct imsgbuf));
		imsg_init(info[i]->volume, volsock[i]);
	}

	return info;
}

void
destroy_procinfo(procinfo_t *info)
{
	imsg_clear(info->volume);

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
	pthread_t xth, dth, vth;

	procinfo = get_procinfo();

	if (fork() == 0) {
		destroy_procinfo(procinfo[0]);
		volume_main(procinfo[1]);
	}
	destroy_procinfo(procinfo[1]);

	info = get_info();
	info->procinfo = procinfo[0];

	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	init_output();

	pthread_create(&xth, NULL, watch_for_x_changes, info);
	pthread_create(&dth, NULL, watch_for_datetime_changes, info);
	/* pthread_create(&vth, NULL, volume_watch_for_changes, info); */

	for (;;)
		sleep(60 * 60 * 24);

	return 0;
}
