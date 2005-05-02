#include <glib.h>
#include <string.h>		/* strlen */
#include "netdb.h"		/* command_t, netdb_func_t */
#include "netdb-db.h"		/* command_t, netdb_func_t */
#include "validate.h"

/* NOTE: every function is expected to allocate memory to out_msg,
 * 			 bacause it will be freed after its invocation */

int
cmd_quit (gchar ** tab, guint count, gchar ** out_msg)
{
  *out_msg = g_strdup ("+OK Bye\n");
  return 0;
}

int
cmd_setvar (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg;
  int ret;

  if (count < 3)
  {
    msg = "-ERR Too few arguments\n";
  }
  else
  {
    ret = db_setvar (tab[1], tab[2]);
    if (-1 == ret)
      msg = "-ERR Illegal variable\n";
    else
      msg = "+OK Variable set\n";
  }
  *out_msg = g_strdup (msg);
  return 1;
}

int
cmd_getvar (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg;
  char *value;

  if (count < 2)
  {
    msg = g_strdup ("-ERR Too few arguments\n");
  }
  else
  {
    value = db_getvar (tab[1]);
    if (NULL == value)
      msg = g_strdup ("-ERR Variable not set or illegal\n");
    else
      msg = g_strdup_printf ("+OK %s\n", value);
  }
  *out_msg = msg;
  return 1;
}

int
cmd_host (gchar ** tab, guint count, gchar ** out_msg)
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
    ip = get_std_ip_str (tab[1]);
    mac = get_std_mac_str (tab[2]);
    if (NULL == ip)
      msg = "-ERR Bad ip address\n";
    else if (NULL == mac)
      msg = "-ERR Bad mac address\n";
    else
    {
      ret = db_add_replace_update (ip, mac);
      g_free (ip);
      g_free (mac);
      if (1 == ret)
	msg = "+OK Entry added\n";
      else if (2 == ret)
	msg = "+OK Entry updated\n";
      else if (3 == ret)
	msg = "+OK Last seen time updated\n";
      else
	msg = "-ERR Unknown return code\n";
    }
  }
  *out_msg = g_strdup (msg);
  return 1;
}

int
cmd_remove (gchar ** tab, guint count, gchar ** out_msg)
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
    ip = get_std_ip_str (tab[1]);
    if (NULL == ip)
      msg = "-ERR Bad ip address\n";
    else
    {
      ret = db_remove (ip);
      g_free (ip);
      if (-1 == ret)
	msg = "-ERR No MAC found for given IP\n";
      else
	msg = "+OK Entry removed\n";
    }
  }
  *out_msg = g_strdup (msg);
  return 1;
}

int
cmd_gettime (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg, *ip;
  int time;

  if (count < 2)
  {
    msg = g_strdup ("-ERR Too few arguments\n");
  }
  else
  {
    /* Convert to standard form, catch errors */
    ip = get_std_ip_str (tab[1]);
    if (NULL == ip)
      msg = g_strdup ("-ERR Bad ip address\n");
    else
    {
      time = db_gettime (ip);
      if (-1 == time)
	msg = g_strdup_printf ("-ERR Entry for %s not found\n", ip);
      else
	msg = g_strdup_printf ("+OK %d\n", time);
      g_free (ip);
    }
  }

  *out_msg = msg;
  return 1;
}

int
cmd_getmac (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg, *mac, *ip;

  if (count < 2)
  {
    msg = g_strdup ("-ERR Too few arguments\n");
  }
  else
  {
    /* Convert to standard form, catch errors */
    ip = get_std_ip_str (tab[1]);
    if (NULL == ip)
      msg = g_strdup ("-ERR Bad ip address\n");
    else
    {
      mac = db_getmac (ip);
      if (NULL == mac)
	msg = g_strdup_printf ("-ERR No MAC found for %s\n", ip);
      else
	msg = g_strdup_printf ("+OK %s\n", mac);
      g_free (ip);
    }
  }

  *out_msg = msg;
  return 1;
}

int
cmd_dump (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg;
  char *status;

  msg = db_dump ("%s %s\n");
  if (0 == strlen (msg))
    status = "-ERR No entries\n";
  else
    status = "+OK Dump complete\n";

  msg = g_strconcat (msg, status, NULL);
  *out_msg = msg;
  return 1;
}

/* NULL terminated array of commands */
command_t commands[] = {
  {"quit", cmd_quit}
  ,
  {"host", cmd_host}
  ,
  {"remove", cmd_remove}
  ,
  {"getmac", cmd_getmac}
  ,
  {"gettime", cmd_gettime}
  ,
  {"setvar", cmd_setvar}
  ,
  {"getvar", cmd_getvar}
  ,
  {"dump", cmd_dump}
  ,
  {(char *) NULL, (netdb_func_t *) NULL}
};
