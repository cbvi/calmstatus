#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "types.h"
#include "outfuncs.h"

pthread_mutex_t mut;


void
init_output()
{
	pthread_mutex_init(&mut, NULL);
}

void left() { printf("%s", "%{l}"); }
void right() { printf("%s", "%{r}"); }

void
do_output(xinfo_t *xi)
{
	if (pthread_mutex_trylock(&mut) != 0)
		return;

	left();
	print_workspaces(xi);
	print_title(xi);

	right();
	print_datetime();
	printf("%s", "  ");

	printf("\n");
	fflush(stdout);

	pthread_mutex_unlock(&mut);
}
