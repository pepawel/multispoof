#include <stdio.h>

#define PNAME "ndbexec"

#include "ndb-client.h"

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s socket cmd\n", PNAME);
  printf ("\twhere socket is local netdb socket,\n");
  printf ("\tand cmd is a command to execute.\n");
  return;
}

int
main (int argc, char *argv[])
{
  char *msg, *command, *params, *params_new;
  int ret, tab_count, param_count, i;
  char *socketname;
  char **tab;

  if (argc > 2)
  {
    socketname = argv[1];
    command = argv[2];
  }
  else
  {
    usage ();
    exit (1);
  }

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: ndb_init: %s\n", PNAME, msg);
    return 1;
  }

  /* Join parameters */
  param_count = argc - 3;
  params = (char *) strdup ("");
  for (i = 0; i < param_count; i++)
  {
    params_new = (char *) g_strconcat (params, argv[i + 3], NULL);
    g_free (params);
    params = params_new;
    if (i != (param_count - 1))
    {
      params_new = (char *) g_strconcat (params, " ", NULL);
      g_free (params);
      params = params_new;
    }
  }

  ret = execute_command_long (&tab, command, params);
  g_free (params);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: Command execution failed (%s)\n", PNAME, command);
  }
  else
  {
    tab_count = g_strv_length (tab);
    for (i = 0; i < tab_count; i++)
      printf ("%s\n", tab[i]);
    g_strfreev (tab);
  }

  ndb_cleanup ();
  return 0;
}
