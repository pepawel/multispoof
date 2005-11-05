/* Program name */
#define PNAME "tx"

#include <stdio.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>		/* exit */
#include <unistd.h>		/* read, sleep */
#include <string.h>   /* memset */

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

int
main (int argc, char **argv)
{
  /*
   * libpcap part
   */
  char *dev;			/* Device for packet injection */
  char errbuf[PCAP_ERRBUF_SIZE];	/* Error buffer */
  pcap_t *descr;		/* Pcap handler */

  char *par_name;		/* name which shows in messages in parenthesis */
  int error, ret; /* flags */
  
  u_char packet[MAX_PACKET_SIZE];
  u_int16_t packet_s;


  if (argc < 3)
  {
    usage ();
    exit (1);
  }

  dev = argv[1];
  par_name = argv[2];

  /* Open the device so we can inject frames */
  descr = pcap_open_live (dev, 0, 0, 0, errbuf);
  if (descr == NULL)
  {
    fprintf (stderr, "%s (%s): pcap_open_live: %s\n",
	     PNAME, par_name, errbuf);
    exit (1);
  }
  
  /* Print device name to the user */
  fprintf (stderr, "%s (%s): using device %s\n", PNAME, par_name, dev);

  /* Main loop: get frame from stdin and inject it */
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
	    ret = pcap_inject (descr, packet, packet_s < 60 ? 60 : packet_s);
	    if (-1 == ret)
	    {
	      fprintf (stderr, "%s (%s): packet injection failed: %s\n",
		      PNAME, par_name, pcap_geterr(descr));
	      error = 1;
	      break;
    	}
    }
  }

  /* Clean up */
  pcap_close (descr);

  return 0;
}
