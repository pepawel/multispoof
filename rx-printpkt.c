#include <stdio.h>
#include "rx-printpkt.h"

void print_packet(const u_char *p, const u_int32_t l)
{
	int i;
	for (i = 0; i < l; i++)
	{
		fprintf(stdout, "%.2x", p[i]);
	}
	fprintf(stdout, "\n");
	fflush(stdout);
	return;
}

