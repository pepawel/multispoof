#include <stdio.h>
#include <string.h>		/* memcpy */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */

#define PNAME "arprep"

#include "common.h"
#include "printpkt.h"
#include "getpkt.h"
#include "ndb-client.h"
#include "netheaders.h"

/* Gets ip from arp request.
 * On success ip is filled with address extracted from arp request
 * and 1 is returned. On error -1 is returned.
 * NOTE: Prints error messages on stderr. */
int
get_ip_from_arp_request (ip, request, request_s)
     struct in_addr *ip;
     u_char *request;
     u_int16_t request_s;
{
  const ethernet_packet_t *ethernet;
  const arp_packet_t *arp;
  int ethernet_s, arp_s;

  /* Casting magic: manipulate packet data in structured way. */
  ethernet_s = sizeof (ethernet_packet_t);
  ethernet = (ethernet_packet_t *) request;
  arp_s = sizeof (arp_packet_t);
  arp = (arp_packet_t *) (request + ethernet_s);

  /* Check for arp correctness:
   * - is packet truncated? */
  if ((ethernet_s + arp_s) > request_s)
  {
    fprintf (stderr, "%s: truncated arp\n", PNAME);
    return -1;
  }
  /* - is this ARP request? */
  if (0x0100 != arp->ar_op)
  {
    return -1;
  }
  /* - FIXME: other checks - ideally checks should be done
   *          in the same way as in linux kernel to prevent
   *          fingerprinting. */

  /* Extract ip address from packet */
  memcpy (ip, arp->t_ip, 4);

  return 1;
}

/* Fills reply using given MAC and info from given request.
 * Internal function used by create_arp_reply. */
void
_fill_arp_reply (out_reply, out_reply_s, mac, request)
     u_int8_t *out_reply;
     u_int16_t *out_reply_s;
     u_int8_t *mac;
     u_char *request;
{
  ethernet_packet_t *req_ethernet;
  arp_packet_t *req_arp;
  ethernet_packet_t *reply_ethernet;
  arp_packet_t *reply_arp;

  int ethernet_s;

  /* Casting magic: manipulate packet data in a structured way. */
  ethernet_s = sizeof (ethernet_packet_t);

  req_ethernet = (ethernet_packet_t *) request;
  req_arp = (arp_packet_t *) (request + ethernet_s);

  reply_ethernet = (ethernet_packet_t *) out_reply;
  reply_arp = (arp_packet_t *) (out_reply + ethernet_s);


  /* Create arp reply */
  memcpy (reply_ethernet->ether_dhost, req_ethernet->ether_shost, 6);
  memcpy (reply_ethernet->ether_shost, mac, 6);
  reply_ethernet->ether_type = 0x0608;	/* ARP type in little endian */
  reply_arp->ar_hrd = 0x0100;
  reply_arp->ar_pro = 8;
  reply_arp->ar_hln = 6;
  reply_arp->ar_pln = 4;
  reply_arp->ar_op = 0x0200;
  memcpy (reply_arp->s_mac, mac, 6);
  memcpy (reply_arp->s_ip, req_arp->t_ip, 4);
  memcpy (reply_arp->t_mac, req_arp->s_mac, 6);
  memcpy (reply_arp->t_ip, req_arp->s_ip, 4);

  *out_reply_s = sizeof (ethernet_packet_t) + sizeof (arp_packet_t);
  return;
}

/* Extracts ip from request, asks netdb for mac for this ip
 * and creates arp reply. On success returns 1, -1 otherwise. */
int
create_arp_reply (reply, reply_s, request, request_s)
     u_char *reply;
     u_int16_t *reply_s;
     u_char *request;
     u_int16_t request_s;
{
  struct in_addr ip;
  int ret;
  static u_int8_t mac[6];

  /* Validate arp and get ip. */
  ret = get_ip_from_arp_request (&ip, request, request_s);
  if (1 != ret)
  {
    return -1;
  }

  /* Get MAC address */
  ret = ndb_execute_getmac (mac, ip);
  if (-1 == ret)
  {
    fprintf (stderr, "debug: MAC for %s not found\n", inet_ntoa (ip));
    return -1;
  }
  else
  {
    /* Create arp reply */
    _fill_arp_reply (reply, reply_s, mac, request);
    return 1;
  }
}

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s [socket]\n", PNAME);
  printf ("\twhere socket is local netdb socket.\n");
  return;
}

/* FIXME: take common filter-like behavior of cmac and arprep
 *        and put it into a module */
int
main (int argc, char *argv[])
{
  u_char arp_request[MAX_PACKET_SIZE];
  u_int16_t arp_request_s;
  u_char arp_reply[MAX_PACKET_SIZE];
  u_int16_t arp_reply_s;
  char *msg;
  int ret;
  char *socketname;

  if (argc > 1)
    socketname = argv[1];
  else
    socketname = "netdbsocket";

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: ndb_init: %s\n", PNAME, msg);
    return 1;
  }

  while (1)
  {
    ret = get_packet (arp_request, &arp_request_s);
    if (-1 == ret)
    {
      fprintf (stderr, "%s: malformed packet.\n", PNAME);
    }
    else if (0 == ret)
    {
      break;
    }
    else
    {
      ret = create_arp_reply (arp_reply, &arp_reply_s,
			      arp_request, arp_request_s);
      if (ret == 1)
	print_packet (arp_reply, arp_reply_s);
    }
  }

  ndb_cleanup ();
  return 0;
}
