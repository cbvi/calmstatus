/*
 * Copyright (c) 2018 Carlin Bingham <cb@viennan.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/uio.h>

#include <assert.h>
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

	if (pfd[0].revents & POLLERR)
		warn("POLLERR");

	if (imsg_compose(ibuf, cmd, -1, -1, -1, NULL, 0) == -1)
		err(1, "imsg_compose");
	if (imsg_flush(ibuf) == -1)
		err(1, "imsg_flush");
}

enum priv_cmd
priv_get_cmd(struct imsgbuf *ibuf)
{
	return priv_wait_cmd(ibuf, -1);
}

enum priv_cmd
priv_wait_cmd(struct imsgbuf *ibuf, int timeout)
{
	struct imsg imsg;
	struct pollfd pfd[1];
	enum priv_cmd cmd;
	int pr;

	pfd[0].fd = ibuf->fd;
	pfd[0].events = POLLIN;

	if ((pr = poll(pfd, 1, timeout)) == -1)
		err(1, "poll");

	if (pr == 0) {
		assert(timeout > -1);
		return CMD_TRYAGAIN;
	}

	if (pfd[0].revents & POLLERR)
		warnx("POLLERR");

	if (imsg_read(ibuf) == -1)
		errx(1, "imsg_read");

	if ((imsg_get(ibuf, &imsg)) == -1)
		errx(1, "imsg_get");

	if ((imsg.hdr.len - IMSG_HEADER_SIZE) != 0)
		err(1, "priv_get_cmd: msg wrong size");

	cmd = imsg.hdr.type;
	imsg_free(&imsg);

	return cmd;
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
