#include <stdio.h>
#include <sys/types.h> /* socket */
#include <sys/socket.h> /* socket, inet_ntoa */
#include <sys/un.h>
#include <string.h> /* strerror */
#include <errno.h> /* errno */
#include <glib.h> /* g_strsplit */
#include <net/ethernet.h> /* ETHER_ADDR_LEN */
#include <netinet/in.h> /* struct in_addr, inet_ntoa */
#include <arpa/inet.h> /* inet_ntoa */
#include <libnet.h> /* libnet_hex_aton */

#define PNAME "cmac"

#include "common.h"
#include "rx-printpkt.h"
#include "tx-getpkt.h"

/* FIXME: this function is copied from glib source (because I use
 * 				old version of glib, which doesn't have it). */
#if ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION < 6))
#ifndef _GLIB_HACK_
#define _GLIB_HACK_
guint
g_strv_length (gchar **str_array)
{
  guint i = 0;

  g_return_val_if_fail (str_array != NULL, 0);

  while (str_array[i])
    ++i;

  return i;
}
#endif
#endif

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

/* Executes command with arg on netdb using s socket, allocates out_buf
 * for reply. If you want to execute command with multiple arguments
 * pass them as one, space separated string.
 * Returns 1 on success, 0 on error, -1 on protocol error.
 * NOTE: For convenience, out_buf is allocated always - even on
 *       protocol error. */
int execute_command(s, out_buf, command, arg)
	FILE *s;
	char **out_buf;
	char *command;
	char *arg;
{
	static char tmp_buf[128];
	gchar **tab;
	int result;
	
	fprintf(s, "%s %s\n", command, arg);
	fflush(s);
	fgets(tmp_buf, sizeof(tmp_buf), s);
	tab = g_strsplit(tmp_buf, " ", 2);
	if (g_strv_length(tab) < 2)
	{
		*out_buf = g_strdup("Protocol error");
		result = -1;
	}
	else
	{
		*out_buf = g_strdup(tab[1]);
		g_strfreev(tab);
		g_strstrip(*out_buf);
	
		result = (tmp_buf[0] == '+' ? 1 : 0);
	}
	return result;
}

/* Changes mac in packet (of packet_s size) using established
 * connection s to netdb daemon.
 * Returns 1 on success, -1 otherwise. */
int change_mac(FILE *s, u_char *packet, u_int16_t packet_s)
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
	ret = execute_command(s, &buf, command, ip_str);
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
	int sfd, len;
	struct sockaddr_un a;
	char *socketname;
	FILE *s;
	char tmp[128];

	socketname = "netdbsocket";
	p_mode = "change"; /* FIXME: use command line switches to set mode*/

	/* FIXME: add setorgmac/getorgmac command to netdb
	 *        - this will simplify cmac algorithm */

	/* Connect to netdb */
	sfd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (-1 == sfd)
	{
		fprintf(stderr, "%s (%s): socket: %s\n",
						PNAME, p_mode, strerror(errno));
		return 1;
	}

	a.sun_family = AF_UNIX;
	len = sizeof(a.sun_path);
	strncpy(a.sun_path, socketname, len);
	
	ret = connect(sfd, (struct sockaddr *) &a, len);
	if (-1 == ret)
	{
		fprintf(stderr, "%s (%s): connect: %s\n",
						PNAME, p_mode, strerror(errno));
		return 1;
	}

	s = fdopen(sfd, "r+");
	/* Get welcome message */
	fgets(tmp, sizeof(tmp), s);

	while(1)
	{
		ret = get_packet(packet, &packet_s);
		if (ret == -1)
		{
			fprintf(stderr, "%s (%s): malformed packet.\n", PNAME, p_mode);
		}
		else
		{
			ret = change_mac(s, packet, packet_s);
			if (ret == 1) print_packet(packet, packet_s);
		}
	}

	fclose(s);
	return 1;
}
