#ifndef _PRIVSEP
#define _PRIVSEP

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <stdint.h>
#include <imsg.h>

enum priv_cmd {
	CMD_OUTPUT_DO,
	CMD_VOLUME_LEVEL,
	CMD_VOLUME_MUTE,
	CMD_DESKTOP_CURRENT,
	CMD_DESKTOP_WINDOWS,
	CMD_DESKTOP_TITLE,
	CMD_STOP_RIGHT_NOW,
	CMD_GOODBYE,
};

enum priv_res {
	RES_VOLUME_LEVEL,
	RES_VOLUME_MUTE,
	RES_DESKTOP_CURRENT,
	RES_DESKTOP_WINDOWS,
	RES_DESKTOP_TITLE,
};

void priv_send_cmd(struct imsgbuf *, enum priv_cmd);
enum priv_cmd priv_get_cmd(struct imsgbuf *);
void priv_send_res(struct imsgbuf *, enum priv_res, const void *, uint16_t);
void priv_get_res(struct imsgbuf *, struct imsg *);
void priv_socketpair(int *);

#endif
