#include <stdio.h>
#include <string.h>		/* memcpy */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */
#include <glib.h>		/* g_strv_length */
#include <stdlib.h> /* exit, atoi, free */
#include <unistd.h> /* sleep */

#define PNAME "scanarp"

#include "common.h"
#include "printpkt.h"
#include "ndb-client.h"
#include "netheaders.h"
#include "validate.h"

/* Fills packet with arp request, out_packet_s with arp size.
 * Uses t_ip/s_ip/s_mac for target ip/source ip/source mac. */
void
create_arp_request (packet, out_packet_s, t_ip, s_ip, s_mac)
     u_char *packet;
     int *out_packet_s;
     struct in_addr t_ip;
     struct in_addr s_ip;
     u_char *s_mac;
{
  ethernet_packet_t *req_ethernet;
  arp_packet_t *req_arp;

  int ethernet_s;

  ethernet_s = sizeof (ethernet_packet_t);
  req_ethernet = (ethernet_packet_t *) packet;
  req_arp = (arp_packet_t *) (packet + ethernet_s);

  /* Calculate size */
  *out_packet_s = sizeof (ethernet_packet_t) + sizeof (arp_packet_t);
  /* Create arp request */
  memcpy (req_ethernet->ether_dhost, "\xff\xff\xff\xff\xff\xff", 6);
  memcpy (req_ethernet->ether_shost, s_mac, 6);
  req_ethernet->ether_type = 0x0608;	/* ARP type in little endian */
  req_arp->ar_hrd = 0x0100;
  req_arp->ar_pro = 8;
  req_arp->ar_hln = 6;
  req_arp->ar_pln = 4;
  req_arp->ar_op = 0x0100;
  memcpy (req_arp->s_mac, s_mac, 6);
  memcpy (req_arp->s_ip, &s_ip, 4);
  memcpy (req_arp->t_mac, "\x0\x0\x0\x0\x0\x0", 6);
  memcpy (req_arp->t_ip, &t_ip, 4);

  return;
}

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s socket int ip mac\n", PNAME);
  printf ("\twhere socket is local netdb socket, int is scan interval\n");
  printf ("\tip is default scan ip and mac is default scan mac.\n");
  return;
}

/* FIXME: take common filter-like behavior of cmac and deta
 *        and put it into a module */
int
main (int argc, char *argv[])
{
  u_char packet[MAX_PACKET_SIZE];
  int packet_s;

  char *msg;
  int ret, i;
  char *socketname;
  int interval;

  /* Array of hosts */
  struct in_addr *tab = NULL;
  int count = 0;

  /* Data needed for creating ARP requests */
  struct in_addr s_ip;
  u_char *s_mac;
  int mac_len;

  if (argc < 5)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];
  interval = atoi (argv[2]);
  /* FIXME: next version should add random delays between arp
   * requests, but keep interval constant. Also there should be
   * no way to fingerprint interval. */
  /* FIXME: next version should send requests in random order. */
  /* FIXME: next version should use one of enabled entries for
   * source IP and MAC. Use of provided source IP-MAC pair only
   * when no entry is enabled. */
  ret = inet_aton (argv[3], &s_ip);
  if (0 == ret)
  {
    fprintf (stderr, "%s: Invalid ip address\n", PNAME);
    exit (1);
  }
  s_mac = libnet_hex_aton (argv[4], &mac_len);
  if (NULL == s_mac)
  {
    fprintf (stderr, "%s: Invalid mac address\n", PNAME);
    exit (1);
  }

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: ndb_init: %s\n", PNAME, msg);
    return 1;
  }

  while (1)
  {
    ret = fetch_host_tab (&tab, &count);
    if (1 == ret)
    {
      for (i = 0; i < count; i++)
      {
	create_arp_request (packet, &packet_s, tab[i], s_ip, s_mac);
	print_packet (packet, packet_s);
      }
      free (tab);
    }
    else
      fprintf (stderr, "%s: fetching host tab failed\n", PNAME);
    sleep (interval);
  }

  free (s_mac);
  ndb_cleanup ();
  return 0;
}
