#include <pthread.h>
#include <unistd.h>

#include "calmstatus.h"

int
main()
{
	info_t *info;
	pthread_t xth, dth, vth;

	info = get_info();

	init_output();

	pthread_create(&xth, NULL, watch_for_x_changes, info);
	pthread_create(&dth, NULL, watch_for_datetime_changes, info);
	pthread_create(&vth, NULL, watch_for_volume_changes, info);

	for (;;)
		sleep(60 * 60 * 24);

	return 0;
}
