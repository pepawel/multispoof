#include <glib.h>
#include <string.h>		/* strlen */
#include <stdlib.h>		/* atoi */
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
	msg = "+OK Age of entry updated\n";
      else
	msg = "-ERR Unknown return code\n";
    }
  }
  *out_msg = g_strdup (msg);
  return 1;
}

int
cmd_do (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg, *ip;
  int ret;

  if (count < 3)
  {
    msg = "-ERR Too few arguments\n";
  }
  else
  {
    /* Check if ip is in correct format, and rewrite
     * to standard format if possible */
    ip = get_std_ip_str (tab[2]);
    if (NULL == ip)
      msg = "-ERR Bad ip address\n";
    else
    {
      if (0 == strcmp (tab[1], "enable"))
	ret = db_change_enabled (ip, 1);
      else if (0 == strcmp (tab[1], "disable"))
	ret = db_change_enabled (ip, 0);
      else if (0 == strcmp (tab[1], "start-test"))
	ret = db_start_stop_test (ip, 1);
      else if (0 == strcmp (tab[1], "stop-test"))
	ret = db_start_stop_test (ip, 0);
      else
	ret = -2;

      g_free (ip);
      if (-1 == ret)
	msg = "-ERR Operation failed - entry not found\n";
      else if (-2 == ret)
	msg = "-ERR Unrecognized operation\n";
      else
	msg = "+OK Operation succeeded\n";
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
cmd_gethost (gchar ** tab, guint count, gchar ** out_msg)
{
  char *msg, *mac, *ip;
  int enabled, cur_test, ret;
  time_t age, test_age;

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
      ret = db_gethost (&mac, &age, &test_age, &enabled, &cur_test, ip);
      if (-1 == ret)
	msg = g_strdup_printf ("-ERR No entry found for %s\n", ip);
      else
	msg = g_strdup_printf ("+OK %s %ld %ld %s %s\n",
			       mac, age, test_age,
			       enabled ? "enabled" : "disabled",
			       cur_test ? "test" : "idle");
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
    status = "+OK No entries\n";
  else
    status = "+OK Dump complete\n";

  *out_msg = g_strconcat (msg, status, NULL);
  free (msg);
  return 1;
}

/* NULL terminated array of commands */
command_t commands[] = {
  {"quit", cmd_quit}		/* Closes client connection. */
  ,
  {"host", cmd_host}		/* Adds, replaces entry. Updates last active. */
  ,
  {"do", cmd_do}		/* Sets enabled or cur_test flag for entry. */
  ,
  {"remove", cmd_remove}	/* Removes entry from db. */
  ,
  {"gethost", cmd_gethost}	/* Returns entry for given ip. */
  ,
  {"setvar", cmd_setvar}	/* Sets given variable with given value. */
  ,
  {"getvar", cmd_getvar}	/* Gets value of given variable. */
  ,
  {"dump", cmd_dump}		/* Dumps all ip addresses. */
  ,
  {(char *) NULL, (netdb_func_t *) NULL}
};
