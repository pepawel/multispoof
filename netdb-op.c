#include <glib.h>
#include "netdb.h"

/* NOTE: every function is expected to allocate memory to out_msg,
 * 			 bacause it will be freed after its invocation */

int
cmd_quit(gchar **tab, guint count, gchar **out_msg)
{
	*out_msg = g_strdup("+OK Bye\n");
	return 0;
}

int
cmd_add(gchar **tab, guint count, gchar **out_msg)
{
	char* msg;
	if (count < 3)
	{
		msg = "-ERR Too few arguments\n";
	}
	else
	{
		/* FIXME: do actual add to hash table operation */
		msg = "+OK Host added \n";
	}
	*out_msg = g_strdup(msg);
	return 1;
}

command_t commands[] =
{
	{"quit", cmd_quit},
	{"add", cmd_add},
	{(char *) NULL, (netdb_func_t *) NULL}
};
