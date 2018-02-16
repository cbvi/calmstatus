#include <sys/socket.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/uio.h>

#include <err.h>
#include <poll.h>
#include <stdint.h>
#include <imsg.h>

#include "calmstatus.h"
#include "priv.h"

void
priv_send_cmd(struct imsgbuf *ibuf, enum priv_cmd cmd)
{
	struct pollfd pfd[1];

	pfd[0].fd = ibuf->fd;
	pfd[0].events = POLLOUT;

	if (poll(pfd, 1, -1) == -1)
		err(1, "poll");

	if (imsg_compose(ibuf, cmd, -1, -1, -1, NULL, 0) == -1)
		err(1, "imsg_compose");
	if (imsg_flush(ibuf) == -1)
		err(1, "imsg_flush");
}

enum priv_cmd
priv_get_cmd(struct imsgbuf *ibuf)
{
	struct imsg imsg;
	struct pollfd pfd[1];

	pfd[0].fd = ibuf->fd;
	pfd[0].events = POLLIN;

	if (poll(pfd, 1, -1) == -1)
		err(1, "poll");

	if (imsg_read(ibuf) == -1)
		errx(1, "imsg_read");

	if ((imsg_get(ibuf, &imsg)) == -1)
		errx(1, "imsg_get");

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != 0)
		err(1, "priv_get_cmd: msg wrong size");

	return imsg.hdr.type;
}

void
priv_send_res(struct imsgbuf *ibuf, enum priv_res res, 
    const void *data, uint16_t sz)
{
	if (imsg_compose(ibuf, res, -1, -1, -1, data, sz) == -1)
		err(1, "imsg_compose");
	if (imsg_flush(ibuf) == -1)
		err(1, "imsg_flush");
}

void
priv_get_res(struct imsgbuf *ibuf, struct imsg *imsg)
{
	if (imsg_read(ibuf) == -1)
		err(1, "imsg_read");
	if (imsg_get(ibuf, imsg) == -1)
		err(1, "imsg_get");
}

void
priv_socketpair(int *sock)
{
	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, sock) == -1)
		err(1, "socketpair");
}
