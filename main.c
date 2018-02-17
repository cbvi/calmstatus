#include <sys/socket.h>

#include <err.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "calmstatus.h"
#include "priv.h"

static procinfo_t **
get_procinfo()
{
	procinfo_t **info;
	int outsock[2], volsock[2], xsock[2];
	int i;

	info = xcalloc(2, sizeof(procinfo_t *));
	priv_socketpair(outsock);
	priv_socketpair(volsock);
	priv_socketpair(xsock);

	for (i = 0; i < 2; i++) {
		info[i] = xcalloc(1, sizeof(procinfo_t));
		info[i]->output = xcalloc(1, sizeof(struct imsgbuf));
		info[i]->volume = xcalloc(1, sizeof(struct imsgbuf));
		info[i]->xstuff = xcalloc(1, sizeof(struct imsgbuf));
		imsg_init(info[i]->output, outsock[i]);
		imsg_init(info[i]->volume, volsock[i]);
		imsg_init(info[i]->xstuff, xsock[i]);
	}

	return info;
}

void
destroy_procinfo(procinfo_t *info)
{
	imsg_clear(info->output);
	imsg_clear(info->volume);
	imsg_clear(info->xstuff);

	free(info);
}

int
main()
{
	procinfo_t **info;

	info = get_procinfo();

	if (fork() == 0) {
		destroy_procinfo(info[0]);
		return volume_main(info[1]);
	}

	if (fork() == 0) {
		destroy_procinfo(info[0]);
		return xstuff_main(info[1]);
	}

	destroy_procinfo(info[1]);

	if (pledge("stdio", NULL) == -1)
		err(1, "pledge");

	init_output();

	datetime_main(info[0]);

	return output_main(info[0]);
}
