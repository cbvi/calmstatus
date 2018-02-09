#include <stdio.h>
#include <err.h>
#include <time.h>

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
