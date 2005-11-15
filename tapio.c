#define PNAME "tapio"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net/ethernet.h>
#include <sys/time.h>		/* select */
#include <stdlib.h>		/* exit */

/* tun/tap includes */
#include <sys/types.h>		/* open, select */
#include <sys/stat.h>		/* open */
#include <fcntl.h>		/* open */
#include <sys/ioctl.h>		/* ioctl */
#include <unistd.h>		/* read, select */
/* Order must be preserved - first socket.h, then if.h */
#include <sys/socket.h>		/* if.h needs it */
#include <linux/if.h>		/* struct ifreq */
#include <linux/if_tun.h>	/* ifr_flags like IFF_TUN etc. */

#include "common.h"
#include "printpkt.h"
#include "getpkt.h"

/* Ethernet header */
struct sniff_ethernet
{
  u_char ether_dhost[ETHER_ADDR_LEN];	/* Destination host address */
  u_char ether_shost[ETHER_ADDR_LEN];	/* Source host address */
  u_short ether_type;		/* IP? ARP? RARP? etc */
};

#define max(a,b) ((a)>(b) ? (a):(b))
#define TAP_PREFIX_SIZE 4

void
usage ()
{
  printf ("Usage: %s name\n", PNAME);
  printf ("\tWhere name is a name assigned to created tap device.\n");
  return;
}

int
main (int argc, char **argv)
{
  int tap_fd;			/* tap filedescriptor */
  int stdin_fd = fileno (stdin);
  int max_fd;
  struct ifreq ifr;
  u_char tap_packet[MAX_PACKET_SIZE + TAP_PREFIX_SIZE];
  u_int16_t tap_packet_s;
  fd_set fds;
  int ret, error = 1;
  char *tap_dev_name;
  int test_flag=0;

  if (argc < 2)
  {
    usage ();
    exit (1);
  }
  tap_dev_name = argv[1];
  if (argc > 2 && (0 == strcmp("-t", argv[2])))
  {
    test_flag=1;
  }

  /* Prepare tap packet - put ip over ethernet proto in prefix */
  tap_packet[0] = 0;
  tap_packet[1] = 0;
  tap_packet[2] = 8;
  tap_packet[3] = 0;

  /* Open tap device */
  tap_fd = open ("/dev/net/tun", O_RDWR);
  if (tap_fd < 0)
  {
    fprintf (stderr, "%s: opening tap device failed: %s\n",
	     PNAME, strerror (errno));
  }
  else
  {
    /* Set tap parameters */
    memset (&ifr, 0, sizeof (ifr));
    ifr.ifr_flags |= IFF_TAP;
    strcpy (ifr.ifr_name, tap_dev_name);
    ret = ioctl (tap_fd, TUNSETIFF, (void *) &ifr);
    if (ret < 0)
    {
      fprintf (stderr, "%s: setting tap device failed: %s\n",
	       PNAME, strerror (errno));
    }
    else
    {
      if (1 == test_flag)
      {
        error = 0;
      }
      else
      {
        fprintf (stderr, "%s: virtual interface: %s\n", PNAME, ifr.ifr_name);
  
        /* Wait for data on descriptors */
        error = 0;
        max_fd = max (tap_fd, stdin_fd) + 1;
        while (1)
        {
  	FD_ZERO (&fds);
  	FD_SET (tap_fd, &fds);
  	FD_SET (stdin_fd, &fds);
  
  	select (max_fd, &fds, NULL, NULL, NULL);
  
  	if (FD_ISSET (tap_fd, &fds))
  	{
  	  ret = read (tap_fd, tap_packet, MAX_PACKET_SIZE);
  	  if (-1 == ret)
  	  {
  	    fprintf (stderr, "%s: read from tap failed: %s\n",
  		     PNAME, strerror (errno));
  	    error = 1;
  	    break;
  	  }
  	  else
  	  {
  	    print_packet (tap_packet + TAP_PREFIX_SIZE,
  			  ret - TAP_PREFIX_SIZE);
  	  }
  	}
  	if (FD_ISSET (stdin_fd, &fds))
  	{
  	  ret = get_packet (tap_packet + TAP_PREFIX_SIZE, &tap_packet_s);
  	  if (-1 == ret)
  	  {
  	    fprintf (stderr, "%s: malformed input\n", PNAME);
  	    error = 1;
  	    break;
	    }
	    else if (0 == ret)
	    {
	      error = 0;
	      break;
	    }
	    else
	    {
	      ret = write(tap_fd, tap_packet, tap_packet_s + TAP_PREFIX_SIZE);
	      if (-1 == ret)
	      {
	        fprintf (stderr, "%s: writing to tap failed: %s\n",
		        PNAME, strerror (errno));
	        error = 1;
	        break;
	      }
	    }
	  }
        }
      }
    }
  }
  return error;
}
