#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#define PNAME "test-netdb"

#include "../ndb-client.h"

/* Executes gethost command on netdb count times */
void
test_netdb (int count)
{
  int ret;
  char msg[MSG_BUF_SIZE];
  int i;

  for (i = 0; i < count; i++)
  {
    ret = execute_command (msg, "gethost", "192.168.64.1");
    if (1 != ret)
    {
      fprintf (stderr, "execute_command: %s\n", msg);
    }
  }
  return;
}

int
main (int argc, char *argv[])
{
  int ret;
  char *msg;

  if (argc < 3)
  {
    printf ("Usage:\n\t%s socketname iteration_count\n", PNAME);
    return 1;
  }
  ret = ndb_init (argv[1], &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "ndb_init: %s\n", msg);
    return 1;
  }

  /* Let's test netdb */
  test_netdb (atoi (argv[2]));

  ndb_cleanup ();
  return 1;
}
