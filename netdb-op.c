#include <glib.h>
#include "netdb.h" /* command_t, netdb_func_t */
#include "netdb-db.h" /* command_t, netdb_func_t */
#include "validate.h"

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
	char *msg, *mac, *ip;
	int ret;
	if (count < 3)
	{
		msg = "-ERR Too few arguments\n";
	}
	else
	{
		/* Check if ip and mac are in correct format, and rewrite
		 * to standard format if possible */
		ip = get_std_ip_str(tab[1]);
		mac = get_std_mac_str(tab[2]);
		if (NULL == ip)
			msg = "-ERR Bad ip address\n";
		else if (NULL == mac)
			msg = "-ERR Bad mac address\n";
		else
		{
			ret = db_add(ip, mac);
			g_free(ip);
			g_free(mac);
			if (-1 == ret)
				msg = "-ERR Entry already exist\n";
			else
				msg = "+OK Entry added\n";
		}
	}
	*out_msg = g_strdup(msg);
	return 1;
}

int
cmd_remove(gchar **tab, guint count, gchar **out_msg)
{
	char *msg, *ip;
	int ret;
	if (count < 2)
	{
		msg = "-ERR Too few arguments (ip address needed)\n";
	}
	else
	{
		/* Check if ip is in correct format, and rewrite
		 * to standard format if possible */
		ip = get_std_ip_str(tab[1]);
		if (NULL == ip)
			msg = "-ERR Bad ip address\n";
		else
		{
			ret = db_remove(ip);
			g_free(ip);
			if (-1 == ret)
				msg = "-ERR Entry not found\n";
			else
				msg = "+OK Entry removed\n";
		}
	}
	*out_msg = g_strdup(msg);
	return 1;
}

int
cmd_getmac(gchar **tab, guint count, gchar **out_msg)
{
	char *msg, *mac, *ip;
	if (count < 2)
	{
		msg = g_strdup("-ERR Too few arguments\n");
	}
	else
	{
		/* Convert to standard form, catch errors */
		ip = get_std_ip_str(tab[1]);
		if (NULL == ip)
			msg = g_strdup("-ERR Bad ip address\n");
		else
		{
			mac = db_getmac(ip);
			g_free(ip);
			if (NULL == mac)
				msg = g_strdup("-ERR Entry not found\n");
			else
				msg = g_strdup_printf("+OK %s\n", mac);
		}
	}
	
	*out_msg = msg;
	return 1;
}

int
cmd_dump(gchar **tab, guint count, gchar **out_msg)
{
	char *msg;
	char *status;
	
	msg = db_dump("%s %s\n");
	if (0 == strlen(msg))
		status = "-ERR No entries\n";
	else
		status = "+OK Dump complete\n";

	msg = g_strconcat(msg, status, NULL);
	*out_msg = msg;
	return 1;
}
	
command_t commands[] =
{
	{"quit", cmd_quit},
	{"add", cmd_add},
	{"remove", cmd_remove},
	{"getmac", cmd_getmac},
	{"dump", cmd_dump},
	{(char *) NULL, (netdb_func_t *) NULL}
};
