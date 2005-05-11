#include <stdio.h>
#include <string.h>		/* memcpy */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */
#include <glib.h>		/* g_strv_length */

#define PNAME "scanarp"

#include "common.h"
#include "printpkt.h"
#include "ndb-client.h"
#include "netheaders.h"

int
fetch_host_tab(struct in_addr **out_tab, int *out_count)
{
  char **text_tab;
  struct in_addr *ip_tab;
  int count, i, ret;
  char *ptr;
  
  ret = execute_command_long(&text_tab, "dump", "");
  if (1 == ret)
  {
    count = g_strv_length(text_tab);
    ip_tab = g_malloc(sizeof(struct in_addr) * count);
    for (i = 0; i < count; i++)
    {
      ptr = index(text_tab[i], ' ');
      *ptr = '\0';
      ret = inet_aton(text_tab[i], &(ip_tab[i]));
      if (ret == 0)
      {
        fprintf(stderr, "%s: Invalid address in db (%s)\n",
            PNAME, text_tab[i]);
        exit(1);
      }
    }
    g_strfreev(text_tab);
  }
  else
    return -1;
  
  *out_tab = ip_tab;
  *out_count = count;
  return 1;
}

void
create_arp_request(packet, out_packet_s, t_ip, s_ip, s_mac)
  u_int8_t *packet;
  int *out_packet_s;
  struct in_addr t_ip;
  struct in_addr s_ip;
  u_int8_t *s_mac;
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
  memcpy (req_ethernet->ether_dhost, "\xf1\xf2\xf3\xf4\xf5\xf6", 6);
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
  u_int8_t *s_mac;
  int mac_len;

  if (argc < 5)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];
  interval = atoi (argv[2]);
  /* FIXME: next version should pick IP and MAC randomly from db,
   * and use provided IP-MAC pair only when db data is not usable. */
  ret = inet_aton(argv[3], &s_ip);
  if (0 == ret)
  {
    fprintf(stderr, "%s: Invalid ip address\n", PNAME);
    exit(1);
  }
  s_mac = libnet_hex_aton(argv[4], &mac_len);
  if (NULL == s_mac)
  {
    fprintf(stderr, "%s: Invalid mac address\n", PNAME);
    exit(1);
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
    ret = fetch_host_tab(&tab, &count);
    if (1 == ret)
    {
      for (i = 0; i < count; i++)
      {
        create_arp_request(packet, &packet_s, tab[i], s_ip, s_mac);
	      print_packet (packet, packet_s);
      }
      free(tab);
    }
    else
      fprintf(stderr, "%s: fetching host tab failed\n", PNAME);
    sleep (interval);
  }

  free(s_mac);
  ndb_cleanup ();
  return 0;
}
