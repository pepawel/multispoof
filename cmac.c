#include <stdio.h>
#include <glib.h>		/* g_free */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */
#include <libnet.h>		/* libnet_hex_aton */

#define PNAME "cmac"

#include "common.h"
#include "printpkt.h"
#include "getpkt.h"
#include "ndb-client.h"
#include "validate.h"
#include "netheaders.h"

/* Spoof mode indicators */
char *p_mode_string;		/* for human */
int spoof_mode;			/* for program */

/* Default mac - used in uspoof mode */
u_int8_t *default_mac;

/* Changes mac in packet (of packet_s size) according to global
 * spoof_mode variable.
 * If spoof mode is 1 and packet src ip exists in netdb, then src mac
 * is changed to value from netdb.
 * If spoof mode is 0 and packet dst ip exists in netdb, then dst mac
 * is changed to global default_mac.
 * Returns 1 on success, -1 otherwise.
 * NOTE: Function printf error messages itself. */
/* FIXME: modularize this function - it is too long and ugly
 *        - do it like in arprep. */
int
change_mac (u_char * packet, u_int16_t packet_s)
{
  char *ip_str;
  char *buf;
  int ret, result, mac_len;
  u_int8_t *mac;
  struct in_addr ip_val;

  const ethernet_packet_t *ethernet;
  const ip_packet_t *ip;
  int ethernet_s, ip_s;

  /* Check for ip truncation */
  ethernet_s = sizeof (ethernet_packet_t);
  ethernet = (ethernet_packet_t *) (packet);
  ip_s = sizeof (ip_packet_t);
  if ((ethernet_s + ip_s) > packet_s)
  {
    /* Ip is truncated. */
    fprintf (stderr, "%s (%s): truncated ip\n", PNAME, p_mode_string);
    return -1;
  }

  /* Extract ip address from packet and convert to string */
  ip = (ip_packet_t *) (packet + ethernet_s);
  /* Depending on spoof mode extract src or dst address */
  ip_val = (spoof_mode ? ip->ip_src : ip->ip_dst);
  ip_str = inet_ntoa (ip_val);

  /* Get mac from netdb */
  ret = execute_command (&buf, "getmac", ip_str);
  if (1 == spoof_mode)
  {
    if (1 == ret)
    {
      /* Convert mac in string form to array of bytes */
      mac = libnet_hex_aton (buf, &mac_len);
      /* Do actual substitution */
      /* mac == 6 bytes */
      memcpy ((void *) ethernet->ether_shost, mac, 6);
      g_free (mac);
      result = 1;
    }
    else
    {
      fprintf (stderr, "%s (%s): debug error: %s\n",
	       PNAME, p_mode_string, buf);
      result = -1;
    }
  }
  else
  {
    if (1 == ret)
    {
      memcpy ((void *) ethernet->ether_dhost, default_mac, 6);
      result = 1;
    }
    else
    {
      fprintf (stderr, "%s (%s): debug error: %s\n",
	       PNAME, p_mode_string, buf);
      result = -1;
    }
  }
  g_free (buf);
  return result;
}

/* Gets default mac from netdb and sets global variable default_mac.
 * On success returns 1, -1 otherwise.
 * Funtions will not return until "getvar defmac" return "+OK" status.
 * NOTE: Function prints error messages itself. */
int
update_default_mac ()
{
  char *mac, *valid_mac;
  int ret, result, mac_len;

  ret = -1;
  while (1 != ret)
  {
    /* FIXME: here should be debug option check - if debug,
     *        print message: "Trying to get defmac..." */
    ret = execute_command (&mac, "getvar", "defmac");
    usleep (100000);
  }
  /* Convert mac in string form to byte array */
  valid_mac = get_std_mac_str (mac);
  if (NULL == valid_mac)
  {
    fprintf (stderr, "%s (%s): Default mac not valid\n",
	     PNAME, p_mode_string);
    result = -1;
  }
  else
  {
    default_mac = libnet_hex_aton (valid_mac, &mac_len);
    fprintf (stderr, "%s (%s): Using %s as default mac\n",
	     PNAME, p_mode_string, valid_mac);
    g_free (valid_mac);
    result = 1;
  }

  g_free (mac);
  return result;
}

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s mode\n", PNAME);
  printf ("where mode can be:\n");
  printf ("\tspoof - for changing source mac\n");
  printf ("\tunspoof - for changing destination mac\n");
  return;
}

int
main (int argc, char *argv[])
{
  u_char packet[MAX_PACKET_SIZE];
  u_int16_t packet_s;
  char *msg;
  int ret;
  char *socketname;

  socketname = "netdbsocket";

  if (argc < 2)
  {
    usage ();
    return 1;
  }

  if (0 == strcmp (argv[1], "spoof"))
    spoof_mode = 1;
  else if (0 == strcmp (argv[1], "unspoof"))
    spoof_mode = 0;
  else
  {
    usage ();
    return 1;
  }
  p_mode_string = argv[1];

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s (%s): ndb_init: %s\n", PNAME, p_mode_string, msg);
    return 1;
  }

  /* If in unspoof mode - get default mac address */
  if (0 == spoof_mode)
  {
    ret = update_default_mac ();
    if (-1 == ret)
    {
      ndb_cleanup ();
      return 1;
    }
  }

  while (1)
  {
    ret = get_packet (packet, &packet_s);
    if (-1 == ret)
    {
      fprintf (stderr, "%s (%s): malformed packet.\n", PNAME, p_mode_string);
    }
    else if (0 == ret)
    {
      break;
    }
    else
    {
      ret = change_mac (packet, packet_s);
      if (ret == 1)
	print_packet (packet, packet_s);
    }
  }

  ndb_cleanup ();
  return 0;
}
