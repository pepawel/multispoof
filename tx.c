#include <stdio.h>
#include <string.h>

/* libnet */
#include <libnet.h>

#define PNAME "tx"

#include "common.h"
#include "getpkt.h"
#include "netheaders.h"

void
usage ()
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "\t%s iface name\n", PNAME);
  fprintf (stderr, "Where iface is an interface to inject packets\n");
  fprintf (stderr, "into, and name is a string which shows in messages.\n");
  return;
}

/* Send packet (byte array) p of size s using libnet context l.
 * FIXME: free ptag after last invocation of this func */
/* FIXME: add error strings */
int
send_packet (l, p, s)
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
  payload = p + sizeof (ethernet_packet_t);
  payload_s = s - sizeof (ethernet_packet_t);
  dst = p + 0;
  src = p + 6;
  type = p[13] + (p[12] * 0x100);

  /* Let's construct packet */
  ptag = libnet_build_ethernet (dst, src, type, payload, payload_s, l, ptag);	/* reusing ptag */
  if (-1 == ptag)
  {
    result = -1;
  }
  else
  {
    /* Send the packet */
    ret = libnet_write (l);
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
main (int argc, char **argv)
{
  libnet_t *l;
  u_char packet[MAX_PACKET_SIZE];
  u_int16_t packet_s;
  char errbuf[LIBNET_ERRBUF_SIZE];
  int error = 1;
  int ret;
  char *par_name;

  if (argc < 3)
  {
    usage ();
    exit (1);
  }

  par_name = argv[2];

  /* Start libnet */
  l = libnet_init (LIBNET_LINK, argv[1], errbuf);
  if (NULL == l)
  {
    fprintf (stderr, "%s (%s): libnet_init: %s", PNAME, par_name, errbuf);
  }
  else
  {
    fprintf (stderr, "%s (%s): using device %s\n",
	     PNAME, par_name, libnet_getdevice (l));

    error = 0;
    while (1)
    {
      ret = get_packet (packet, &packet_s);
      if (-1 == ret)
      {
	fprintf (stderr, "%s (%s): malformed packet\n", PNAME, par_name);
      }
      else if (0 == ret)
      {
	error = 0;
	break;
      }
      else
      {
	/* Add padding if neccessary. Padding is added by ethernet
	 * driver, but:
	 * - I want to see correct packet on network interface
	 *   using tcpdump locally
	 * - I do not trust ethernet drivers these days ;-) */
	if (packet_s < 60)
	{
	  memset (packet + packet_s, 0, 60 - packet_s);
	}
	ret = send_packet (l, packet, packet_s < 60 ? 60 : packet_s);
	if (-1 == ret)
	{
	  fprintf (stderr, "%s (%s): sending failed: %s\n",
		   PNAME, par_name, "FIXME");
	  error = 1;
	  break;
	}
      }
    }
    /* Shutdown libnet */
    libnet_destroy (l);
  }
  return error;
}
