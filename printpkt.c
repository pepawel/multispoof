#include <stdio.h>
#include "printpkt.h"

/* Prints packet (byte array) p of length l in hex. */
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

