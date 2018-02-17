#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "calmstatus.h"

void *
watch_for_datetime_changes(void *arg)
{
	time_t clock;
	struct tm *tp;
	procinfo_t *info;

	info = (procinfo_t *)arg;

	if ((clock = time(NULL)) == -1)
		err(1, "time");

	if ((tp = localtime(&clock)) == NULL)
		err(1, "localtime");

	sleep(60 - tp->tm_sec);

	for (;;) {
		do_output(info);
		sleep(60);
	}
}

void
print_datetime()
{
	time_t clock;
	struct tm *tp;
	char buf[32];

	if ((clock = time(NULL)) == -1)
		err(1, "time");

	if ((tp = localtime(&clock)) == NULL)
		err(1, "localtime");

	if (strftime(buf, sizeof(buf), "%a %d %b %H:%M", tp) == 0)
		errx(1, "strftime");

	printf("%s", buf);
}

int
datetime_main(procinfo_t *info)
{
	pthread_t thr;

	pthread_create(&thr, NULL, watch_for_datetime_changes, info);

	return 0;
}
