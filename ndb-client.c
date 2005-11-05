#include <stdio.h>
#include <sys/types.h>		/* socket */
#include <unistd.h>		/* close */
#include <sys/socket.h>
#include <arpa/inet.h>		/* struct in_addr */
#include <sys/un.h>
#include <string.h>		/* strerror */
#include <errno.h>		/* errno */
#include <glib.h>		/* g_strsplit */
#include <stdlib.h>		/* strtoul */

#define PNAME "ndb-client"

#include "validate.h"
#include "ndb-client.h"

/* Global socket for communication with netdb.
 * Use only by functions in this module. */
FILE *s = NULL;

/* Executes command with arg on netdb, fills msg_buf with reply.
 * If you want to execute command with multiple arguments
 * pass them as one, space separated string.
 * Returns 1 on success, 0 on error, -1 on fatal or protocol error.
 * NOTE: Uses global s socket. */
int
execute_command (msg_buf, command, arg)
     char *msg_buf;
     char *command;
     char *arg;
{
  static char tmp_buf[MSG_BUF_SIZE];
  int result;
  char *msg_start, *nl_ptr;

  /* Sanity check */
  if (NULL == s)
  {
    g_strlcpy (msg_buf, "Not connected to netdb", MSG_BUF_SIZE);
    result = -1;
  }
  else
  {
    fprintf (s, "%s %s\n", command, arg);
    fflush (s);
    fgets (tmp_buf, MSG_BUF_SIZE, s);

    msg_start = index (tmp_buf, ' ');
    nl_ptr = index (tmp_buf, '\n');
    if ((NULL == msg_start) || (NULL == nl_ptr))
    {
      g_strlcpy (msg_buf, "Protocol error", MSG_BUF_SIZE);
      result = -1;
    }
    else
    {
      *nl_ptr = '\0';
      g_strlcpy (msg_buf, msg_start + 1, MSG_BUF_SIZE);

      result = (tmp_buf[0] == '+' ? 1 : 0);
    }
  }
  return result;
}

int
execute_command_long (out_vector, command, arg)
     char ***out_vector;
     char *command;
     char *arg;
{
  static char tmp_buf[128];
  gchar **tab;
  int result, count;
  char *long_string, *long_string_new;

  /* Sanity check */
  if (NULL == s)
  {
    return -1;
  }

  fprintf (s, "%s %s\n", command, arg);
  fflush (s);
  long_string = g_strdup ("");
  count = 0;
  while (1)
  {
    fgets (tmp_buf, sizeof (tmp_buf), s);
    if ('+' == tmp_buf[0])
    {
      result = 1;
      break;
    }
    else if ('-' == tmp_buf[0])
    {
      result = -1;
      break;
    }
    else
    {
      long_string_new = g_strconcat (long_string, tmp_buf, NULL);
      g_free (long_string);
      long_string = long_string_new;
      count++;
    }
  }

  if (-1 == result)
  {
    g_free (long_string);
  }
  else
  {
    tab = g_strsplit (long_string, "\n", count);
    if (0 != count)
      g_strstrip (tab[count - 1]);

    g_free (long_string);
    *out_vector = tab;
  }
  return result;
}


/* Executes gethost command on netdb. On success fills mac, enabled
 * flag and returns 1. On failure returns -1. */
int
ndb_execute_gethost (mac, out_age, out_test_age, out_enabled, out_cur_test,
		     ip)
     u_char *mac;
     time_t *out_age;
     time_t *out_test_age;
     int *out_enabled;
     int *out_cur_test;
     struct in_addr ip;
{
  int ret, mac_len, result;
  char buf[MSG_BUF_SIZE];
  char *mac_ptr, *age_ptr, *test_age_ptr, *enabled_ptr, *cur_test_ptr;
  u_char *mac_tmp;

  ret = execute_command (buf, "gethost", inet_ntoa (ip));
  if (1 == ret)
  {
    /* Find substrings */
    mac_ptr = buf;
    if ((age_ptr = index (buf, ' ')) &&
	(test_age_ptr = index (age_ptr + 1, ' ')) &&
	(enabled_ptr = index (test_age_ptr + 1, ' ')) &&
	(cur_test_ptr = index (enabled_ptr + 1, ' ')))
    {
      /* Isolate strings */
      *age_ptr = '\0';
      *test_age_ptr = '\0';
      *enabled_ptr = '\0';
      *cur_test_ptr = '\0';

      /* Export values */
      if (0 == strcmp ("enabled", enabled_ptr + 1))
	*out_enabled = 1;
      else
	*out_enabled = 0;
      if (0 == strcmp ("test", cur_test_ptr + 1))
	*out_cur_test = 1;
      else
	*out_cur_test = 0;

      *out_age = strtoul (age_ptr + 1, NULL, 0);
      *out_test_age = strtoul (test_age_ptr + 1, NULL, 0);
      /* Convert mac in string form to array of bytes */
      mac_tmp = libnet_hex_aton (mac_ptr, &mac_len);
      /* ethernet mac == 6 bytes */
      memcpy (mac, mac_tmp, 6);
      g_free (mac_tmp);
      result = 1;
    }
    else
    {
      fprintf (stderr, "[%s]: gethost returned malformed output 1.\n", PNAME);
      result = -1;
    }
  }
  else
    result = -1;
  return result;
}

int
ndb_execute_host (ip, mac)
     struct in_addr ip;
     u_char *mac;
{
  int ret, result;
  char buf[MSG_BUF_SIZE];
  char *params;

  params = g_strconcat (inet_ntoa (ip), " ", mac_ntoa (mac), NULL);
  ret = execute_command (buf, "host", params);
  g_free (params);
  if (1 == ret)
  {
    if (0 == strcmp (buf, "Entry added"))
      result = 1;
    else if (0 == strcmp (buf, "Entry updated"))
      result = 2;
    else if (0 == strcmp (buf, "Age of entry updated"))
      result = 3;
    else
      result = -1;
  }
  else
    result = -1;
  return result;
}

int
ndb_execute_do (char *op, struct in_addr ip)
{
  char buf[MSG_BUF_SIZE];
  char params[64];

  sprintf (params, "%s %s", op, inet_ntoa (ip));
  return execute_command (buf, "do", params);
}

/* Fetches all IP addresses from tab and fill out_tab array.
 * out_count is filled with array size.
 * Array should be freed after use.
 * Returns 1 on success, -1 otherwise. */
int
fetch_host_tab (struct in_addr **out_tab, int *out_count)
{
  char **text_tab;
  struct in_addr *ip_tab;
  int count, i, ret;

  ret = execute_command_long (&text_tab, "dump", "");
  if (1 == ret)
  {
    count = g_strv_length (text_tab);
    ip_tab = g_malloc (sizeof (struct in_addr) * count);
    for (i = 0; i < count; i++)
    {
      ret = inet_aton (text_tab[i], &(ip_tab[i]));
      if (ret == 0)
      {
	fprintf (stderr, "%s: Invalid address in db (%s)\n",
		 PNAME, text_tab[i]);
	exit (1);
      }
    }
    g_strfreev (text_tab);
  }
  else
    return -1;

  *out_tab = ip_tab;
  *out_count = count;
  return 1;
}

/* Establishes connection with netdb on socket socketname.
 * Returns 1 on success, -1 otherwise and set out_error to point
 * to static char array with error message (no need to free it). */
#define ERR_BUF_SIZE 1024
int
ndb_init (char *socketname, char **out_error)
{
  int ret;
  int sfd, len;
  struct sockaddr_un a;
  char tmp[128];
  static char error_buf[ERR_BUF_SIZE];
  int result = -1;

  /* Connect to netdb */
  sfd = socket (PF_UNIX, SOCK_STREAM, 0);
  if (-1 == sfd)
  {
    snprintf (error_buf, ERR_BUF_SIZE, "[%s]: socket: %s\n",
	      PNAME, strerror (errno));
  }
  else
  {
    a.sun_family = AF_UNIX;
    len = sizeof (a.sun_path);
    strncpy (a.sun_path, socketname, len);

    ret = connect (sfd, (struct sockaddr *) &a, len);
    if (-1 == ret)
    {
      snprintf (error_buf, ERR_BUF_SIZE, "[%s]: connect: %s\n",
		PNAME, strerror (errno));
      close (sfd);
    }
    else
    {
      s = fdopen (sfd, "r+");
      if (NULL == s)
      {
	snprintf (error_buf, ERR_BUF_SIZE, "[%s]: fdopen: %s\n",
		  PNAME, strerror (errno));
	close (sfd);
      }
      else
      {
	/* Eat welcome message */
	fgets (tmp, sizeof (tmp), s);
	result = 1;
      }
    }
  }
  if (-1 == result)
    *out_error = error_buf;
  return result;
}

/* Closes connection no netdb. */
void
ndb_cleanup ()
{
  fclose (s);
  return;
}
