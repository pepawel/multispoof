#include <stdio.h>
#include <glib.h> /* g_free */
#include <net/ethernet.h> /* ETHER_ADDR_LEN */
#include <netinet/in.h> /* struct in_addr, inet_ntoa */
#include <arpa/inet.h> /* inet_ntoa */
#include <libnet.h> /* libnet_hex_aton */

#define PNAME "cmac"

#include "common.h"
#include "rx-printpkt.h"
#include "tx-getpkt.h"
#include "ndb-client.h"

/* FIXME: move this structs to separate file (used in rx and cmac) */

/* Ethernet header */
struct sniff_ethernet
{
  u_char ether_dhost[ETHER_ADDR_LEN];	/* Destination host address */
  u_char ether_shost[ETHER_ADDR_LEN];	/* Source host address */
  u_short ether_type;		/* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip
{
#if BYTE_ORDER == LITTLE_ENDIAN
  u_int ip_hl:4,		/* header length */
    ip_v:4;			/* version */
#if BYTE_ORDER == BIG_ENDIAN
  u_int ip_v:4,			/* version */
    ip_hl:4;			/* header length */
#endif
#endif				/* not _IP_VHL */
  u_char ip_tos;		/* type of service */
  u_short ip_len;		/* total length */
  u_short ip_id;		/* identification */
  u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
  u_char ip_ttl;		/* time to live */
  u_char ip_p;			/* protocol */
  u_short ip_sum;		/* checksum */
  struct in_addr ip_src, ip_dst;	/* source and dest address */
};

char *p_mode;

/* Changes mac in packet (of packet_s size).
 * Returns 1 on success, -1 otherwise. */
int change_mac(u_char *packet, u_int16_t packet_s)
{
	char *command;
	char *ip_str;
	char *buf;
	int ret, result, mac_len;
	u_int8_t *mac;

	const struct sniff_ethernet *ethernet;
	const struct sniff_ip *ip;
	int ethernet_s, ip_s;
	
	/* Check for ip truncation */
	ethernet_s = sizeof(struct sniff_ethernet);
	ethernet = (struct sniff_ethernet *) (packet);
	ip_s = sizeof(struct sniff_ip);
	if ((ethernet_s + ip_s) > packet_s)
	{
		/* Ip is truncated. */
		fprintf(stderr, "%s (%s): truncated ip\n", PNAME, p_mode);
		return -1;
	}
	
	/* Extract ip address from packet and convert to string */
	ip = (struct sniff_ip *) (packet + ethernet_s);
	ip_str = inet_ntoa(ip->ip_src);

	/* Get mac from netdb */
	command = "getmac";
	ret = execute_command(&buf, command, ip_str);
	if (1 == ret)
	{
		/* Convert mac in string form to array of bytes */
		mac = libnet_hex_aton(buf, &mac_len);
		/* Do actual substitution */
		memcpy((void *) ethernet->ether_shost, mac, 6); /* mac == 6 bytes */
		g_free(mac);
		result = 1;
	}
	else
	{
		fprintf(stderr, "%s (%s): error: %s\n", PNAME, p_mode, buf);
		result = -1;
	}
	g_free(buf);
	return result;
}

int
main(int argc, char *argv[])
{
	u_char packet[MAX_PACKET_SIZE];
	u_int16_t packet_s;
	int ret;
	char *socketname;
	char *msg;

	socketname = "netdbsocket";
	p_mode = "change"; /* FIXME: use command line switches to set mode*/

	/* FIXME: add setorgmac/getorgmac command to netdb
	 *        - this will simplify cmac algorithm */

	/* Connect to netdb */
	ret = ndb_init(socketname, &msg);
	if (-1 == ret)
	{
		fprintf(stderr, "%s (%s): ndb_init: %s\n", PNAME, p_mode, msg);
		return 1;
	}

	while(1)
	{
		ret = get_packet(packet, &packet_s);
		if (ret == -1)
		{
			fprintf(stderr, "%s (%s): malformed packet.\n", PNAME, p_mode);
		}
		else
		{
			ret = change_mac(packet, packet_s);
			if (ret == 1) print_packet(packet, packet_s);
		}
	}

	ndb_cleanup();
	return 1;
}
