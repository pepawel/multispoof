#ifndef _NETHEADERS_H_
#define _NETHEADERS_H_

#ifndef ETHER_ADDR_LEN
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#endif

/* Ethernet header */
typedef struct
{
  u_char ether_dhost[ETHER_ADDR_LEN];	/* Destination host address */
  u_char ether_shost[ETHER_ADDR_LEN];	/* Source host address */
  u_short ether_type;		/* IP? ARP? RARP? etc */
} sniff_ethernet_t;

/* IP header */
typedef struct
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
} sniff_ip_t;

#endif
