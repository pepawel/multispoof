#ifndef _TX_GETPKT_H_
#define _TX_GETPKT_H_

#include <sys/types.h>

int hex2byte(u_int8_t* tab,	char *string,	u_int16_t *size);
int get_packet(u_char *p,	u_int16_t *s);
#endif
