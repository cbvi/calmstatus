#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "calmstatus.h"

static info_t *
get_info()
{
	info_t *info;

	info = xcalloc(1, sizeof(info_t));

	info->xinfo = get_xinfo();
	info->soundinfo = volume_get_soundinfo();

	return info;
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
	pthread_t xth, dth, vth;

	info = get_info();

	init_output();

	pthread_create(&xth, NULL, watch_for_x_changes, info);
	pthread_create(&dth, NULL, watch_for_datetime_changes, info);
	pthread_create(&vth, NULL, volume_watch_for_changes, info);

	for (;;)
		sleep(60 * 60 * 24);

	return 0;
}
