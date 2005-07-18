#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>		/* unlink, select, exit */
#include <stdlib.h>		/* unlink, select, exit */
#include <fcntl.h>		/* fcntl */
#include <sys/time.h>		/* select */
#include <glib.h>
#include <signal.h>

#include "netdb.h"
#include "netdb-db.h"

#define PNAME "netdb"

#define max(a,b) ((a)>(b) ? (a):(b))

/* FIXME: limit number of connection to prevent fd_set overflow */

/* Global variables, to make clean_up function work
 * when called from signal handling routine. */
int serversocket;		/* Socket for accepting new clients */
char *socketname;		/* File name of the socket */
char *cachefile;		/* Name of the cache file */

/* Use list of FILE pointers to populate fd_set given as fds_pointer */
int
populate_fd_set (GSList * list, fd_set * fds_pointer)
{
  GSList *li;

  for (li = list; NULL != li; li = li->next)
  {
    FD_SET (fileno (li->data), fds_pointer);
  }
  return 1;
}

/* Returns fd with highest number from list */
int
get_max_fd (GSList * list)
{
  GSList *li;
  int max = -1;
  int fd;

  for (li = list; NULL != li; li = li->next)
  {
    fd = fileno (li->data);
    if (fd > max)
      max = fd;
  }
  return max;
}

/* Accepts connection on serversocket and adds it as FILE* to list.
 * NOTE: the list pointer is modified */
int
accept_and_add (GSList ** out_list, int serversocket)
{
  int clientsocket;
  FILE *s;

  clientsocket = accept (serversocket, NULL, NULL);
  if (clientsocket == -1)
  {
    fprintf (stderr, "%s: accept: %s\n", PNAME, strerror (errno));
    return -1;
  }

  s = fdopen (clientsocket, "r+");
  if (s == NULL)
  {
    fprintf (stderr, "%s: fdopen: %s\n", PNAME, strerror (errno));
    return -1;
  }
  /* Adding opened connection to list (FILE is pointer,
   * so we can do this) */
  *out_list = g_slist_append (*out_list, s);

  /* Send banner */
  fprintf (s, "+OK Welcome\n");
  fflush (s);
  return 1;
}

/* Finds command given by name in global commands[] array.
 * Returns index of command found, or -1 when there is no
 * such command. */
int
find_command (char *name)
{
  int i;
  int result = -1;

  for (i = 0; NULL != commands[i].name; i++)
    if (0 == strcmp (name, commands[i].name))
    {
      result = i;
      break;
    }
  return result;
}

/* This function launch command specified in tab using
 * associated function, leaves result in out_msg.
 * It returns 0 if connection should be closed, 1 otherwise. */
int
dispatch_command (gchar ** tab, char **out_msg)
{
  guint num;
  int c;
  int result = 1;

  num = g_strv_length (tab);
  /* We are intereseted only in non-zero array of commands */
  if (0 != num)
  {
    /* FIXME: use array of structers with function pointers */
    c = find_command (tab[0]);
    if (-1 == c)
    {
      *out_msg = g_strdup ("-ERR No such command\n");
      result = 1;
    }
    else
    {
      result = (*(commands[c].func)) (tab, num, out_msg);
    }
  }
  else
  {
    *out_msg = g_strdup ("");
    result = 1;
  }
  return result;
}

/* Reads line from given f stream, tokenizes it and executes
 * using dispatch_command().
 * Returns result of command execution, or 0 when end of file. */
#define MAX_COMMAND_SIZE 128
int
get_and_serve (FILE * f)
{
  char *command;
  char *output = NULL;
  gchar **ctab;
  int result = 1;

  command = g_malloc (MAX_COMMAND_SIZE * sizeof (char));
  fgets (command, MAX_COMMAND_SIZE, f);
  if (0 != feof (f))
  {
    /* User closed the connection */
    fprintf (f, "+OK Bye\n");
    fflush (f);
    result = 0;			/* 0 means - close this connection */
  }
  else
  {
    /* Parse string */
    g_strstrip (command);
    ctab = g_strsplit (command, " ", 4);

    /* Execute command */
    result = dispatch_command (ctab, &output);

    /* Send message to user */
    fprintf (f, "%s", output);
    fflush (f);

    g_free (output);
    g_strfreev (ctab);
  }
  g_free (command);
  return result;
}

/* Serves new and existing connections on given socket.
 * Connections are maintained as a linked list of streams.
 * New connections are served using accept_and_add(),
 * existing connections are handled with get_and_serve().
 * 
 * This function doesn't return.
 * FIXME: what about memory dealocation, files closing on CTRL-C
 *        or someting like that? */
int
serve_connections (int serversocket)
{
  /* select stuff */
  fd_set fds;
  int max_fd;
  int ret;
  GSList *fd_list = NULL;
  GSList *li;

  while (1)
  {
    /* fprintf (stderr, "%s: (debug) loop\n", PNAME); */
    FD_ZERO (&fds);
    FD_SET (serversocket, &fds);
    populate_fd_set (fd_list, &fds);
    max_fd = max (get_max_fd (fd_list), serversocket) + 1;

    ret = select (max_fd, &fds, NULL, NULL, NULL);
    if (-1 == ret)
    {
      fprintf (stderr, "%s: select: %s\n", PNAME, strerror (errno));
      return -1;
    }
    else if (0 == ret)
    {
      fprintf (stderr, "%s: select returned 0\n", PNAME);
      return -1;
    }
    else
    {
      if (FD_ISSET (serversocket, &fds))
      {
	/* New connection */
	/* fprintf (stderr, "%s: (debug) new connection\n", PNAME); */
	accept_and_add (&fd_list, serversocket);
      }
      else
      {
	/* New data on existing connections.
	 * Need to find out which socket is ready to be read */
	/* fprintf (stderr, "%s: (debug) data\n", PNAME); */
	for (li = fd_list; NULL != li; li = li->next)
	{
	  if (FD_ISSET (fileno (li->data), &fds))
	  {
	    ret = get_and_serve (li->data);
	    if (0 == ret)
	    {
	      /* It's time to close the connection */
	      fclose (li->data);
	      /* We can't delete list element here, instead
	       * we mark it to delete later. */
	      li->data = NULL;
	    }
	  }
	}
	/* Delete closed connections from list */
	fd_list = g_slist_remove_all (fd_list, NULL);
      }
    }
  }
  g_slist_foreach (fd_list, (GFunc) g_free, NULL);
  g_slist_free (fd_list);
  return 1;
}

/* Signal handler - removes socket and frees resources. */
void
clean_up (int sig)
{
  int i, ret;
  char *dump, *mac;
  char **tab;
  time_t tmp1, tmp2;
  int tmp3, tmp4;
  FILE *f;

  f = fopen (cachefile, "w");
  if (NULL != f)
  {
    /* Fetch dump from db and split into tab of ips. */
    dump = db_dump ();
    g_strstrip (dump);
    tab = g_strsplit (dump, "\n", 0);
    g_free (dump);

    /* For each ip get its info. */
    for (i = 0; NULL != tab[i]; i++)
    {
      ret = db_gethost (&mac, &tmp1, &tmp2, &tmp3, &tmp4, tab[i]);
      if (1 == ret)
	fprintf (f, "host %s %s\n", tab[i], mac);
      else
	fprintf (stderr, "%s: gethost error when saving cache\n", PNAME);
    }
    g_strfreev (tab);
    fclose (f);
  }
  else
  {
    fprintf (stderr, "%s: Cache saving failed: %s\n",
	     PNAME, strerror (errno));
  }

  db_free ();
  close (serversocket);

  ret = unlink (socketname);
  if (ret == -1)
  {
    fprintf (stderr, "%s: unlink: %s\n", PNAME, strerror (errno));
  }
  exit (0);
}

/* Feeds db from cachefile. It is done in the same way
 * as when reading commands from sockets. File is open
 * in "r" mode, so "+OK" responsed will be silently ignored. */
void
load_db_from_cache ()
{
  FILE *f;

  f = fopen (cachefile, "r");
  if (NULL != f)
  {
    while (get_and_serve (f));
    fclose (f);
  }
  return;
}

void
usage ()
{
  printf ("Usage: %s socket cache [-f]\n", PNAME);
  printf ("Where socket is a name of local socket to listen on,\n");
  printf ("cache is a name of file where cache is stored.\n");
  printf ("When -f option is present, netdb will flush saved cache.\n");
  return;
}

int
main (int argc, char *argv[])
{
  /* socket stuff */
  struct sockaddr_un a;
  int ret;
  int len, flush_cache;

  if (argc < 3)
  {
    usage ();
    exit (1);
  }
  socketname = argv[1];
  cachefile = argv[2];
  if (argc > 3 && (0 == strcmp (argv[3], "-f")))
    flush_cache = 1;
  else
    flush_cache = 0;

  serversocket = socket (PF_UNIX, SOCK_STREAM, 0);
  if (serversocket == -1)
  {
    fprintf (stderr, "%s: socket: %s\n", PNAME, strerror (errno));
    return 1;
  }

  a.sun_family = AF_UNIX;
  len = sizeof (a.sun_path);
  strncpy (a.sun_path, socketname, len);

  ret = bind (serversocket, (struct sockaddr *) &a, len);
  if (ret == -1)
  {
    fprintf (stderr, "%s: bind: %s\n", PNAME, strerror (errno));
    return 1;
  }

  ret = listen (serversocket, 3);
  if (ret == -1)
  {
    fprintf (stderr, "%s: listen: %s\n", PNAME, strerror (errno));
    return 1;
  }

  /* Create db */
  db_init ();

  /* Fill db with data from cache */
  if (0 == flush_cache)
    load_db_from_cache ();

  /* Catch signals which could stop the process */
  signal (SIGTERM, clean_up);
  signal (SIGINT, clean_up);
  signal (SIGQUIT, clean_up);
  signal (SIGABRT, clean_up);

  /* Ignore SIGPIPE caused by disconnecting user */
  signal (SIGPIPE, SIG_IGN);

  fprintf (stderr, "%s: Listening on %s\n", PNAME, socketname);

  /* Serve connections and return on error */
  serve_connections (serversocket);

  /* Clean up */
  clean_up (-1);

  /* clean_up() doesn't return so this code never executes */
  return 1;
}
