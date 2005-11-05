#include <stdio.h>
#include <string.h>		/* memcpy */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */
#include <glib.h>
#include <stdlib.h> /* exit, free */

#define PNAME "multispoof-dump"

#include "common.h"
#include "ndb-client.h"
#include "validate.h"		/* mac_ntoa */

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s socket\n", PNAME);
  printf ("\twhere socket is local netdb socket.\n");
  return;
}

int
main (int argc, char *argv[])
{
  char *msg;
  char msg_buf[128];
  int ret, i;
  char *socketname;

  /* Array of hosts */
  struct in_addr *tab = NULL;
  int count = 0;
  char *ip;

  if (argc < 2)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: ndb_init: %s\n", PNAME, msg);
    return 1;
  }

  ret = fetch_host_tab (&tab, &count);
  if (1 == ret)
  {
    for (i = 0; i < count; i++)
    {
      ip = inet_ntoa(tab[i]);
	    ret = execute_command (msg_buf, "gethost", ip);
      printf("%s %s\n", ip, msg_buf);
    }
    free (tab);
  }
  else
    fprintf (stderr, "%s: fetching host tab failed\n", PNAME);

  ndb_cleanup ();
  return 0;
}
