/*
 * Based on sniffer.c	(c) 2002 Tim Carstens GPL
 */

/* Program name */
#define PNAME "rx"
#define _BSD_SOURCE 1

#include <stdio.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* #include <net/if.h> */
/* #include <netinet/if_ether.h> */
#include <net/ethernet.h>

#include <stdlib.h> /* exit */
#include <unistd.h> /* read, sleep */

#include "rx-printpkt.h"

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

/* Called on every sniffed packet by pcap_loop */
void
got_packet(u_char *args, const struct pcap_pkthdr *header,
					 const u_char *packet)
{
  /* Define pointers for packet's attributes */
  const struct sniff_ethernet *ethernet;	/* The ethernet header */
  const struct sniff_ip *ip;	/* The IP header */
  /* And define the size of the structures we're using */
  int size_ethernet = sizeof (struct sniff_ethernet);

  /* Define our packet's attributes */
  ethernet = (struct sniff_ethernet *) (packet);
  ip = (struct sniff_ip *) (packet + size_ethernet);
	
	/* Send packet */
	print_packet(packet, header->len);
	
	return;
}

void usage()
{
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s iface\n", PNAME);
	fprintf(stderr,"Where iface is a interface to sniff on.\n");
	return;
}

int
main (int argc, char **argv)
{
	/*
	 * libpcap part
	 */
  char *dev;											/* Sniffing device */
  char errbuf[PCAP_ERRBUF_SIZE];	/* Error buffer */
  pcap_t *descr;									/* Sniff handler */

  struct bpf_program fp;					/* hold compiled program */
  bpf_u_int32 maskp;							/* subnet mask */
  bpf_u_int32 netp;								/* ip */
  char filter_app[] = "ip";
	
  /* Set device for sniffing */
	if (argc < 2)
	{
		usage();
		exit(1);
	}
	
  dev = argv[1];
  pcap_lookupnet (dev, &netp, &maskp, errbuf);

  /* Print device to the user */
  fprintf(stderr, "%s: listening on %s\n", PNAME, dev);

  /* Open the device so we can spy */
  descr = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);
  if (descr == NULL)
	{
		fprintf(stderr, "%s: pcap_open_live: %s\n", PNAME, errbuf);
		exit(1);
	}

  /* Apply the rules */
  if (-1 == pcap_compile(descr, &fp, filter_app, 0, netp))
	{
		fprintf(stderr, "%s: pcap_compile error\n", PNAME);
		exit(1);
	}
  if (-1 == pcap_setfilter (descr, &fp))
	{
		fprintf(stderr, "%s: pcap_setfilter error\n", PNAME);
		exit(1);
	}

  /* Now we can set our callback function */
  pcap_loop(descr, -1, got_packet, NULL);
  
	/* Clean up */
	pcap_close(descr);
	
  return 0;
}

