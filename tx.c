#include <stdio.h>
#include <string.h>

/* libnet */
#include <libnet.h>

#define PNAME "tx"

#include "common.h"
#include "tx-getpkt.h"

/* Ethernet header */
struct sniff_ethernet
{
  u_int8_t ether_dhost[ETHER_ADDR_LEN];	/* Destination host address */
  u_int8_t ether_shost[ETHER_ADDR_LEN];	/* Source host address */
  u_int16_t ether_type;		/* IP? ARP? RARP? etc */
};

void usage()
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s iface\n", PNAME);
	fprintf(stderr,
					"Where iface is a interface to inject packets into.\n");
	return;
}

/* Send packet. FIXME: free ptag after last invocation of this func */
/* FIXME: add error strings */
int send_packet(l, p, s)
	libnet_t *l;
	u_char *p;
	u_int16_t s;
{
		u_int8_t *dst, *src, *payload;
		u_int16_t type;
		u_int16_t payload_s;
		static libnet_ptag_t ptag = 0;
		int ret, result = -1;
		
		/* Pointers magic FIXME: remove magic constants use casts */
		payload = p + sizeof(struct sniff_ethernet);
		payload_s = s - sizeof(struct sniff_ethernet);
		dst = p + 0;
		src = p + 6;
		type = p[13] + (p[12] * 0x100);

		/* Let's construct packet */
		ptag = libnet_build_ethernet(
				dst,
				src,
				type,
				payload,
				payload_s,
				l,
				ptag); /* reusing ptag */
		if (-1 == ptag)
		{
			result = -1;
		}
		else
		{
			/* Send the packet */
			ret = libnet_write(l);
			if (-1 == ret)
			{
				result = -1;
			}
			else
			{
				result = 1;
			}
		}
	return result;
}

int
main(int argc, char **argv)
{
  libnet_t *l;
	u_char packet[MAX_PACKET_SIZE];
	u_int16_t packet_s;
	char errbuf[LIBNET_ERRBUF_SIZE];
	int error = 1;
	int ret;

	if (argc < 2)
	{
		usage();
		exit(1);
	}
	
	/* Start libnet */
	l = libnet_init(LIBNET_LINK, argv[1], errbuf);
	if (NULL == l)
	{
		fprintf(stderr, "%s: libnet_init: %s", PNAME, errbuf);
	}
	else
	{
		fprintf(stderr, "%s: using device %s\n", PNAME,
						libnet_getdevice(l));

		error = 0;
		while(1)
		{
			ret = get_packet(packet, &packet_s);
			if (ret == -1)
			{
				fprintf(stderr, "%s: malformed input\n", PNAME);
				error = 1;
				break;
			}
			else
			{
				ret = send_packet(l, packet, packet_s);
				if (ret == -1)
				{
					fprintf(stderr, "%s: sending failed: %s\n", PNAME, "FIXME");
					error = 1;
					break;
				}
			}
		}
		/* Shutdown libnet */
		libnet_destroy(l);
	}
	return error;
}
