#include <glib.h>
#include "netdb.h" /* command_t, netdb_func_t */
#include "netdb-db.h" /* command_t, netdb_func_t */

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
	int ret;
	if (count < 3)
	{
		msg = "-ERR Too few arguments\n";
	}
	else
	{
		/* Check if ip and mac are in correct format */
		ret = db_add(tab[1], tab[2]);
		if (-1 == ret)
			msg = "-ERR Entry already exist\n";
		else
			msg = "+OK Entry added \n";
	}
	*out_msg = g_strdup(msg);
	return 1;
}

int
cmd_getmac(gchar **tab, guint count, gchar **out_msg)
{
	char *msg;
	char *mac;
	if (count < 2)
	{
		msg = g_strdup("-ERR Too few arguments\n");
	}
	else
	{
		/* No need to check if ip is in correct format,
		 * because if it was it won't be found */
		mac = db_getmac(tab[1]);
		if (NULL == mac)
			msg = g_strdup("-ERR Entry not found\n");
		else
			msg = g_strdup_printf("+OK %s\n", mac);
	}
	
	*out_msg = msg;
	return 1;
}

int
cmd_dump(gchar **tab, guint count, gchar **out_msg)
{
	*out_msg = db_dump("+OK %s %s\n");
	return 1;
}
	
command_t commands[] =
{
	{"quit", cmd_quit},
	{"add", cmd_add},
	{"getmac", cmd_getmac},
	{"dump", cmd_dump},
	{(char *) NULL, (netdb_func_t *) NULL}
};
