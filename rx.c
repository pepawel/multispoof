/*
 * Based on sniffer.c	(c) 2002 Tim Carstens GPL
 */

/* Program name */
#define PNAME "rx"

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

#include "printpkt.h"
#include "netheaders.h"

/* Called on every sniffed packet by pcap_loop */
void
got_packet (u_char * args, const struct pcap_pkthdr *header,
	    const u_char * packet)
{
  /* Define pointers for packet's attributes */
  const ethernet_packet_t *ethernet;	/* The ethernet header */
  const ip_packet_t *ip;	/* The IP header */

  /* And define the size of the structures we're using */
  int size_ethernet = sizeof (ethernet_packet_t);

  /* Define our packet's attributes */
  ethernet = (ethernet_packet_t *) (packet);
  ip = (ip_packet_t *) (packet + size_ethernet);

  /* Send packet */
  print_packet (packet, header->len);

  return;
}

void
usage ()
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "\t%s iface filter name\n", PNAME);
  fprintf (stderr, "Where iface is a interface to sniff on,\n");
  fprintf (stderr, "      filter is a pcap filter and name\n");
  fprintf (stderr, "      is a string which shows in messages.");
  return;
}

int
main (int argc, char **argv)
{
  /*
   * libpcap part
   */
  char *dev;			/* Sniffing device */
  char errbuf[PCAP_ERRBUF_SIZE];	/* Error buffer */
  pcap_t *descr;		/* Sniff handler */

  struct bpf_program fp;	/* hold compiled program */
  bpf_u_int32 maskp;		/* subnet mask */
  bpf_u_int32 netp;		/* ip */
  char *filter_app;
  char *par_name;		/* name which shows in messages in parenthesis */

  /* Set device for sniffing */
  if (argc < 4)
  {
    usage ();
    exit (1);
  }

  dev = argv[1];
  filter_app = argv[2];
  par_name = argv[3];
  pcap_lookupnet (dev, &netp, &maskp, errbuf);

  /* Print device to the user */
  fprintf (stderr, "%s (%s): listening on %s\n", PNAME, par_name, dev);

  /* Open the device so we can spy */
  descr = pcap_open_live (dev, BUFSIZ, 1, 0, errbuf);
  if (descr == NULL)
  {
    fprintf (stderr, "%s (%s): pcap_open_live: %s\n",
	     PNAME, par_name, errbuf);
    exit (1);
  }

  /* Set direction - we are interested only in incoming packets */
  if (-1 == pcap_direction (descr, D_IN))
  {
    fprintf (stderr, "%s (%s): pcap_direction error\n", PNAME, par_name);
    exit (1);
  }

  /* Apply the rules */
  if (-1 == pcap_compile (descr, &fp, filter_app, 0, netp))
  {
    fprintf (stderr, "%s (%s): pcap_compile error\n", PNAME, par_name);
    exit (1);
  }
  if (-1 == pcap_setfilter (descr, &fp))
  {
    fprintf (stderr, "%s (%s): pcap_setfilter error\n", PNAME, par_name);
    exit (1);
  }

  /* Now we can set our callback function */
  pcap_loop (descr, -1, got_packet, NULL);

  /* Clean up */
  pcap_close (descr);

  return 0;
}
