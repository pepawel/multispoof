#include <stdio.h>
#include <glib.h>

#define PNAME "natman"

#include "ndb-client.h"

char *iptables_binary = "/sbin/iptables";

/* Prints usage. */
void
usage ()
{
  printf ("Usage:\n\t%s chain age interval\n", PNAME);
  printf ("\nwhere\tchain is iptables chain for rules storage,\n");
  printf
    ("\tage is minimal inactivity time when host is considered turned off,\n");
  printf ("\tinterval tells how often netdb is queried.\n");
  return;
}

int
fetch_enabled_list (GSList ** out_list, int age)
{
  GSList *list;
  char **tab;
  int i, count;
  char age_text[21];

  list = *out_list;

  sprintf (age_text, "%d", age);
  execute_command_long (&tab, "listenabled", age_text);

  count = g_strv_length (tab);

  for (i = 0; i < count; i++)
  {
    list = g_slist_append (list, g_strdup (tab[i]));
  }
  g_strfreev (tab);

  *out_list = list;
  return 1;
}

int
compare_lists (GSList * first, GSList * second)
{
  GSList *fli, *sli;
  int result;

  fli = first;
  sli = second;
  while (1)
  {
    if ((NULL == fli) && (NULL == sli))
    {
      result = 0;
      break;
    }
    else if (NULL == fli)
    {
      result = -1;
      break;
    }
    else if (NULL == sli)
    {
      result = -1;
      break;
    }
    else
    {
      if (0 == strcmp (fli->data, sli->data))
      {
	fli = fli->next;
	sli = sli->next;
      }
      else
      {
	result = -1;
	break;
      }
    }
  }
  return result;
}

int
update_nat_rules (GSList * list, char *nf_chain)
{
  GSList *li;
  int i, count, ret;
  char *cmd;

  count = g_slist_length (list);

  fprintf (stderr, "%s: updating nat rules (count: %d)\n", PNAME, count);
  /* Flush rules */
  cmd = g_strdup_printf ("%s -t nat -F %s\n", iptables_binary, nf_chain);
  ret = system (cmd);
  if (-1 == ret)
    fprintf (stderr, "%s: iptables call failed\n", PNAME);
  g_free (cmd);
  /* Add rules */
  for (li = list, i = 0; li != NULL; li = li->next, i++)
  {
    if (1 == count)
    {
      cmd = g_strdup_printf ("%s -t nat -A %s -j SNAT --to-source %s\n",
			     iptables_binary, nf_chain, (char *) li->data);
    }
    else
    {
      cmd =
	g_strdup_printf
	("%s -t nat -A %s -m nth --every %d --packet %d -j SNAT --to-source %s\n",
	 iptables_binary, nf_chain, count, i, (char *) li->data);
    }
    ret = system (cmd);
    if (-1 == ret)
      fprintf (stderr, "%s: iptables call failed\n", PNAME);
    g_free (cmd);
  }
  return 1;
}

int
main (int argc, char *argv[])
{
  char *msg;
  int ret, interval, min_age;
  char *socketname;
  char *nf_chain;
  GSList *new_list, *old_list;

  socketname = "netdbsocket";

  if (argc < 4)
  {
    usage ();
    exit (1);
  }

  nf_chain = argv[1];
  min_age = atoi (argv[2]);
  interval = atoi (argv[3]);

  /* Connect to netdb */
  ret = ndb_init (socketname, &msg);
  if (-1 == ret)
  {
    fprintf (stderr, "%s: ndb_init: %s\n", PNAME, msg);
    return 1;
  }

  old_list = NULL;
  while (1)
  {
    new_list = NULL;
    ret = fetch_enabled_list (&new_list, min_age);
    if (1 == ret)
    {
      ret = compare_lists (new_list, old_list);
      if (0 != ret)
      {
	update_nat_rules (new_list, nf_chain);
	/* new_list becomes old_list */
	if (NULL != old_list)
	{
	  g_slist_foreach (old_list, (GFunc) g_free, NULL);
	  g_slist_free (old_list);
	}
	old_list = new_list;
      }
      else if (NULL != new_list)
      {
	g_slist_foreach (new_list, (GFunc) g_free, NULL);
	g_slist_free (new_list);
      }
      sleep (interval);
    }
  }

  ndb_cleanup ();
  return 0;
}
