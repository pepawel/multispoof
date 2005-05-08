#include <stdio.h>
#include <string.h>		/* memcpy */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */

#define PNAME "deta"

#include "common.h"
#include "printpkt.h"
#include "getpkt.h"
#include "ndb-client.h"
#include "netheaders.h"
#include "validate.h"

/* Minimal age of host to be used. */
int min_age;

void
packet_get_source_mac (mac, packet)
     u_int8_t *mac;
     u_char *packet;
{
  ethernet_packet_t *ethernet = (ethernet_packet_t *) packet;

  memcpy (mac, ethernet->ether_shost, ETHER_ADDR_LEN);
  return;
}

void
ip_get_source_ip (out_ip, packet)
     struct in_addr *out_ip;
     u_char *packet;
{
  ip_packet_t *ip = (ip_packet_t *) (packet + sizeof (ethernet_packet_t));

  *out_ip = ip->ip_src;
  return;
}

void
arp_get_source_ip (out_ip, packet)
     struct in_addr *out_ip;
     u_char *packet;
{
  arp_packet_t *arp = (arp_packet_t *) (packet + sizeof (ethernet_packet_t));

  memcpy (out_ip, arp->s_ip, 4);
  return;
}

void
arp_get_target_ip (out_ip, packet)
     struct in_addr *out_ip;
     u_char *packet;
{
  arp_packet_t *arp = (arp_packet_t *) (packet + sizeof (ethernet_packet_t));

  memcpy (out_ip, arp->t_ip, 4);
  return;
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
  int ret, enabled, age;
  static u_int8_t mac[6];

  arp_get_target_ip (&ip, request);

  /* Get MAC address */
  ret = ndb_execute_gethost (mac, &enabled, &age, ip);
  if ((1 == ret) && (1 == enabled) && (age > min_age))
  {
    /* Create arp reply */
    _fill_arp_reply (reply, reply_s, mac, request);
    return 1;
  }
  else
    return -1;
}

/* Returns ethernet protocol of the packet or -1 on error.*/
int
get_packet_proto (out_proto, packet, packet_s)
     u_short *out_proto;
     u_char *packet;
     u_int16_t packet_s;
{
  const ethernet_packet_t *ethernet;
  u_short type;
  int result;

  ethernet = (ethernet_packet_t *) packet;
  type = ethernet->ether_type;
  if (0x8 == type)		/* IP */
  {
    /* Check for packet truncation. */
    if (sizeof (ethernet_packet_t) + sizeof (ip_packet_t) > packet_s)
      result = -1;
    else
      result = 1;
  }
  else if (0x608 == type)	/* ARP */
  {
    /* Check for packet truncation. */
    if (sizeof (ethernet_packet_t) + sizeof (arp_packet_t) > packet_s)
      result = -1;
    else
    {
      /* FIXME: Check if it is arp request */
      result = 1;
    }
  }
  else
  {
    result = 1;
  }
  if (1 == result)
    *out_proto = type;
  return result;
}

/* FIXME: banned list needs to be global array of ips fetches
 * only one time at initialisation. */
/* Returns 1 if given ip is on banned list. */
int
is_banned (struct in_addr ip)
{
  char *buf, *ptr;
  int ret = -1;

  while (-1 == ret)
  {
    ret = execute_command (&buf, "getvar", "banned");
    if (-1 == ret)
    {
      fprintf (stderr, "%s: Waiting for banned list\n", PNAME);
      free (buf);
      sleep (1);
    }
  }
  ptr = strstr (buf, inet_ntoa (ip));
  free (buf);
  return (NULL == ptr ? -1 : 1);
}

/* Performs various actions on packet and netdb.
 * For detailed explanation see the code.
 * If reply needs to be send reply and reply_s are filled
 * and function returns 1. When no reply needs to be send
 * function returns 0. On error it returns -1. */
int
mangle_packet (reply, reply_s, packet, packet_s)
     u_char *reply;
     u_int16_t *reply_s;
     u_char *packet;
     u_int16_t packet_s;
{
  int ret;
  u_short proto;
  u_int8_t mac[6];
  struct in_addr ip;
  int result = 0;

  /* Find out protocol used by current packet */
  ret = get_packet_proto (&proto, packet, packet_s);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: incorrect packet/frame\n", PNAME);
  }
  else if ((0x8 == proto) || (0x608 == proto))
  {
    /* Get source MAC */
    packet_get_source_mac (mac, packet);
    /* Get source IP */
    if (0x8 == proto)		/* IP */
    {
      ip_get_source_ip (&ip, packet);
    }
    else if (0x608 == proto)	/* ARP */
    {
      arp_get_source_ip (&ip, packet);
    }

    /* Check if IP is not banned in netdb */
    if (1 != is_banned (ip))
    {
      /* IP is legal. Update db. */
      ret = ndb_execute_host (ip, mac);
      if (-1 == ret)
      {
	fprintf (stderr, "%s: Error updating db with %s (%s)\n",
		 PNAME, inet_ntoa (ip), mac_ntoa (mac));
      }
      else if (1 == ret)
      {
	fprintf (stderr, "%s: Adding host %s (%s) to db\n",
		 PNAME, inet_ntoa (ip), mac_ntoa (mac));
      }
      else if (2 == ret)
      {
	fprintf (stderr, "%s: Updating entry for %s with mac %s\n",
		 PNAME, inet_ntoa (ip), mac_ntoa (mac));
      }
      else
      {
	/* Sniffed ip exists in db with correct mac. */
      }

      /* If it was ARP request, serve it. */
      if (0x608 == proto)
      {
	result = create_arp_reply (reply, reply_s, packet, packet_s);
      }
    }
  }
  else
  {
    fprintf (stderr, "%s: Unknown protocol: %x (%d)\n", PNAME, proto, proto);
  }
  return result;
}

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s socket min_age\n", PNAME);
  printf ("\twhere socket is local netdb socket,\n");
  printf ("\tand min_age is minimal age of host to be used.\n");
  return;
}

/* FIXME: take common filter-like behavior of cmac and deta
 *        and put it into a module */
int
main (int argc, char *argv[])
{
  u_char packet[MAX_PACKET_SIZE];
  u_int16_t packet_s;
  u_char reply[MAX_PACKET_SIZE];
  u_int16_t reply_s;

  char *msg;
  int ret;
  char *socketname;

  if (argc < 3)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];
  min_age = atoi (argv[2]);

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: ndb_init: %s\n", PNAME, msg);
    return 1;
  }

  while (1)
  {
    ret = get_packet (packet, &packet_s);
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
      ret = mangle_packet (reply, &reply_s, packet, packet_s);
      if (1 == ret)
	print_packet (reply, reply_s);
    }
  }

  ndb_cleanup ();
  return 0;
}
