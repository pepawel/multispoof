#include <stdio.h>
#include <string.h>		/* memcpy */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */
#include <glib.h>

#define PNAME "conncheck"

#include "common.h"
#include "ndb-client.h"

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s socket int min_age min_test_age\n", PNAME);
  printf ("\twhere socket is local netdb socket, int is db poll\n");
  printf ("\tinterval, min_age is how old host needs to be to be tested,\n");
  printf ("\tand min_test_age is how often do tests.\n");
  return;
}

/* FIXME: take common filter-like behavior of cmac and deta
 *        and put it into a module */
int
main (int argc, char *argv[])
{
  char *msg;
  int ret, i;
  char *socketname;
  int interval;

  /* Array of hosts */
  struct in_addr *tab = NULL;
  int count = 0;

  if (argc < 5)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];
  interval = atoi (argv[2]);
  /* FIXME: next version should add random delays between tests,
   * but keep interval constant. Also there should be no
   * way to fingerprint interval. */
  /* FIXME: next version should send requests in random order. */
  /* FIXME: next version should use one of enabled entries for
   * source IP and MAC. Use of provided source IP-MAC pair only
   * when no entry is enabled. */

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
	fprintf (stderr, "%s: FIXME\n", PNAME);
	FIXME ndb_execute_gethost ();

	if ((age > min_age) && (test_age > min_test_age))
	{
	  ret = ndb_execute_test ("start", ip);
	  if (1 == ret)
	  {
	    ret = test_host (ip etc);

	    if (1 == ret)
	      ndb_execute_enable (ip);
	    else
	      ndb_execute_disable (ip);

	    ret = ndb_execute_test ("stop", ip);
	    if (1 != ret)
	    {
	      fprintf (stderr, "%s: stopping test failed for %s\n",
		       PNAME, inet_ntoa (ip));
	    }
	  }
	  else
	  {
	    fprintf (stderr, "%s: starting test failed for %s\n",
		     PNAME, inet_ntoa (ip));
	  }
	}
      }
      free (tab);
    }
    else
      fprintf (stderr, "%s: fetching host tab failed\n", PNAME);
    sleep (interval);
  }

  ndb_cleanup ();
  return 0;
}
