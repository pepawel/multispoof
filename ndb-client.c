#include <stdio.h>
#include <sys/types.h>		/* socket */
#include <unistd.h>		/* close */
#include <sys/socket.h>
#include <arpa/inet.h>		/* struct in_addr */
#include <libnet.h>		/* libnet_hex_aton */
#include <sys/un.h>
#include <string.h>		/* strerror */
#include <errno.h>		/* errno */
#include <glib.h>		/* g_strsplit */

#define PNAME "ndb-client"

/* FIXME: this function is copied from glib source (because I use
 * 				old version of glib, which doesn't have it). */
#if ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION < 6))
#ifndef _GLIB_HACK_
#define _GLIB_HACK_
guint
g_strv_length (gchar ** str_array)
{
  guint i = 0;

  g_return_val_if_fail (str_array != NULL, 0);

  while (str_array[i])
    ++i;

  return i;
}
#endif
#endif

/* Global socket for communication with netdb.
 * Use only by functions in this module. */
FILE *s = NULL;

/* Executes command with arg on netdb, allocates out_buf
 * for reply. If you want to execute command with multiple arguments
 * pass them as one, space separated string.
 * Returns 1 on success, 0 on error, -1 on fatal or protocol error.
 * NOTE: For convenience, out_buf is allocated always - even on
 *       protocol error.
 * NOTE: Uses global s socket. */
int
execute_command (out_buf, command, arg)
     char **out_buf;
     char *command;
     char *arg;
{
  static char tmp_buf[128];
  gchar **tab;
  int result;

  /* Sanity check */
  if (NULL == s)
  {
    *out_buf = g_strdup ("Not connected to netdb");
    return -1;
  }

  fprintf (s, "%s %s\n", command, arg);
  fflush (s);
  fgets (tmp_buf, sizeof (tmp_buf), s);
  tab = g_strsplit (tmp_buf, " ", 2);
  if (g_strv_length (tab) < 2)
  {
    *out_buf = g_strdup ("Protocol error");
    result = -1;
  }
  else
  {
    *out_buf = g_strdup (tab[1]);
    g_strfreev (tab);
    g_strstrip (*out_buf);

    result = (tmp_buf[0] == '+' ? 1 : 0);
  }
  return result;
}

/* Executes getmac command on netdb. On success fills mac
 * and returns 1. On failure returns -1, leaving mac untouched. */
int
ndb_execute_getmac (u_int8_t * mac, struct in_addr ip)
{
  int ret, mac_len, result;
  char *buf;
  u_int8_t *mac_tmp;

  ret = execute_command (&buf, "getmac", inet_ntoa (ip));
  if (1 == ret)
  {
    /* Convert mac in string form to array of bytes */
    mac_tmp = libnet_hex_aton (buf, &mac_len);
    /* ethernet mac == 6 bytes */
    memcpy (mac, mac_tmp, 6);
    g_free (mac_tmp);
    result = 1;
  }
  else
    result = -1;
  g_free (buf);
  return result;
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
