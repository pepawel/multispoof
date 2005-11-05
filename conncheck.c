#include <stdio.h>
#include <string.h>		/* memcpy */
#include <net/ethernet.h>	/* ETHER_ADDR_LEN */
#include <netinet/in.h>		/* struct in_addr, inet_ntoa */
#include <arpa/inet.h>		/* inet_ntoa */
#include <glib.h>
#include <stdlib.h>		/* system */
#include <sys/types.h>		/* WIFEXITED, WEXITSTATUS */
#include <sys/wait.h>		/* WIFEXITED, WEXITSTATUS */
#include <unistd.h> /* sleep */

#define PNAME "conncheck"

#include "common.h"
#include "ndb-client.h"
#include "validate.h"		/* mac_ntoa */

char *iptables_binary;
char *test_script;

int
test_host (ip, mac, age, test_age, enabled, min_age, min_test_age, ch)
     struct in_addr ip;
     u_char *mac;
     time_t age;
     time_t test_age;
     int enabled;
     int min_age;
     int min_test_age;
     char *ch;
{
  char *cmd;
  int ret, result;

  /* FIXME: DEBUG */
  /*
     fprintf(stderr, "%s: testing host %s\n", PNAME, inet_ntoa(ip));
   */
  /* Add SNAT rule. */
  cmd = g_strdup_printf ("%s -t nat -I %s -j SNAT --to-source %s\n",
			 iptables_binary, ch, inet_ntoa (ip));
  ret = system (cmd);
  free (cmd);

  /* Execute test script with correct enviroment variables. */
  cmd =
    g_strdup_printf
    ("_IP=\"%s\" _MAC=\"%s\" _AGE=\"%ld\" _TEST_AGE=\"%ld\" _ENABLED=\"%d\" _MIN_AGE=\"%d\" _MIN_TEST_AGE=\"%d\" _NF_CHAIN=\"%s\" %s\n",
     inet_ntoa (ip), mac_ntoa (mac), age, test_age, enabled, min_age,
     min_test_age, ch, test_script);
  ret = system (cmd);
  free (cmd);
  if (WIFEXITED (ret))
  {
    if (0 == (WEXITSTATUS (ret)))
      result = 1;
    else
      result = -1;
  }
  else
  {
    fprintf (stderr, "%s: test script exited abnormally (%s)\n",
	     PNAME, inet_ntoa (ip));
    result = -1;
  }

  /* Delete rules from test chain. */
  cmd = g_strdup_printf ("%s -t nat -F %s\n", iptables_binary, ch);
  ret = system (cmd);
  free (cmd);

  return result;
}

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s socket ch int min_age min_test_age script iptables_path\n", PNAME);
  printf ("\twhere socket is local netdb socket, ch is netfilter\n");
  printf ("\tchain to use, int is db poll interval, min_age is how\n");
  printf ("\told host needs to be to be tested, min_test_age is\n");
  printf ("\thow often do tests, script is a path to executable\n");
  printf ("\tfile which is used to test connectivity and\n");
  printf ("\tiptables_path is a path to iptables binary.\n");
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
  int min_age, min_test_age;
  char *nf_chain;

  /* Array of hosts */
  struct in_addr *tab = NULL;
  int count = 0;

  /* gethost stuff */
  u_char mac[6];
  int enabled, cur_test;
  time_t age, test_age;
  struct in_addr ip;

  if (argc < 8)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];
  nf_chain = argv[2];
  interval = atoi (argv[3]);
  min_age = atoi (argv[4]);
  min_test_age = atoi (argv[5]);
  test_script = argv[6];
  iptables_binary = argv[7];
  /* FIXME: next version should add random delays between tests,
   * but keep interval constant. Also there should be no
   * way to fingerprint interval. */
  /* FIXME: next version should do tests in random order. */
  /* FIXME: next version should execute custom scripts after
   * entry is positively tested to emulate dhcp-request
   * or something similiar. */

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
	ip = tab[i];
	ret = ndb_execute_gethost (mac, &age, &test_age, &enabled,
				   &cur_test, ip);

	if ((1 == ret) && (age > min_age) && (test_age > min_test_age))
	{
	  ret = ndb_execute_do ("start-test", ip);
	  if (1 == ret)
	  {
	    ret = test_host (ip, mac, age, test_age, enabled,
			     min_age, min_test_age, nf_chain);
	    if (1 == ret)
	      ndb_execute_do ("enable", ip);
	    else
	      ndb_execute_do ("disable", ip);

	    ret = ndb_execute_do ("stop-test", ip);
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
	/* FIXME: DEBUG */
	/*
	   else
	   fprintf (stderr, "%s: (debug) not testing %s\n",
	   PNAME, inet_ntoa(ip));
	 */
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
