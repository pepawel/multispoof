#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h> /* unlink, select */
#include <fcntl.h> /* fcntl */
#include <sys/time.h> /* select */
#include <glib.h>

#define max(a,b) ((a)>(b) ? (a):(b))

/* FIXME: limit number of connection to prevent fd_set overflow */
																 
/* FIXME: this function is copied from glib source (because I use
 * 				old version of glib, which doesn't have it) */
guint
g_strv_length (gchar **str_array)
{
  guint i = 0;

  g_return_val_if_fail (str_array != NULL, 0);

  while (str_array[i])
    ++i;

  return i;
}
																 
int populate_fd_set(GSList *list, fd_set *fds_pointer)
{
	GSList *li;
	/* FIXME: debug */
	for (li = list; NULL != li; li = li->next)
	{
		printf("added fd %d to fd_set\n", fileno(li->data));
		FD_SET(fileno(li->data), fds_pointer);
	}
	return 1;
}

/* Returns fd with highest number from list */
int get_max_fd(GSList *list)
{
	GSList *li;
	int max = -1;
	int fd;
	for (li = list; NULL != li; li = li->next)
	{
		fd = fileno(li->data); 
		if (fd > max) max = fd;
	}
	return max;
}

/* Accepts connection on serversocket and adds it as FILE* to list.
 * NOTE: the list pointer is modified */
int
accept_and_add(GSList **out_list, int serversocket)
{
	int clientsocket;
	FILE *s;

	clientsocket = accept(serversocket, NULL, NULL);
	if (clientsocket == -1)
	{
		perror("accept");
		return -1;
	}

	s = fdopen(clientsocket, "r+");
	if (s == NULL)
	{
		perror("fdopen");
		return -1;
	}
	/* Adding opened connection to list (FILE is pointer,
	 * so we can do this) */
	*out_list = g_slist_append(*out_list, s);

	/* Send banner */
	fprintf(s, "+OK Welcome\n");
	fflush(s);
	return 1;
}

/* This function launch command specified in tab using
 * associated function, leaves result in out_msg.
 * It returns 0 if connection should be closed, 1 otherwise. */
int
dispatch_command(gchar **tab, char **out_msg)
{
	guint num;
	num = g_strv_length(tab);
	/* We are intereseted only in non-zero array of commands */
	if (0 != num)
	{
		/* FIXME: use array of structers with function pointers */
		*out_msg = g_strdup("-ERR Not implemented\n");
	}
	else
		*out_msg = g_strdup("");
	return 1;
}

#define MAX_COMMAND_SIZE 128
int
get_and_serve(FILE* f)
{
	char *command;
	char *output = NULL;
	gchar **ctab;
	int result = 1;
	
	command = g_malloc(MAX_COMMAND_SIZE*sizeof(char));
	fgets(command, MAX_COMMAND_SIZE, f);
	if (0 != feof(f))
	{
		/* User closed the connection */
		fprintf(f, "+OK Bye\n");
		fflush(f);
		result = 0;
	}
	else
	{
		/* Parse string */
		g_strstrip(command);
		g_ascii_strdown(command, -1);
		ctab = g_strsplit(command, " ", 4);
		
		/* Execute command */
		result = dispatch_command(ctab, &output);
	
		/* Send message to user */
		fprintf(f, "%s", output);
		fflush(f);

		g_free(output);
		g_strfreev(ctab);
	}
	g_free(command);
	return result;
}

int
serve_connections(int serversocket)
{
	/* select stuff */
	fd_set fds;
	int max_fd;
	int ret;
	GSList *fd_list = NULL;
	GSList *li;

	while(1)
	{
		FD_ZERO(&fds);
		FD_SET(serversocket, &fds);
		populate_fd_set(fd_list, &fds);
		max_fd = max(get_max_fd(fd_list), serversocket) + 1;

		ret = select(max_fd, &fds, NULL, NULL, NULL);
		if (-1 == ret)
		{
			perror("select");
			return -1;
		}
		else if (0 == ret)
		{
			fprintf(stderr, "netdb: select returned 0\n");
			return -1;
		}
		else
		{
			if (FD_ISSET(serversocket, &fds))
			{
				/* New connection */
				accept_and_add(&fd_list, serversocket);
			}
			else
			{
				/* New data on existing connections.
				 * Need to find out which socket is ready to be read */
				for (li = fd_list; NULL != li; li = li->next)
				{
					if (FD_ISSET(fileno(li->data), &fds))	
					{
						ret = get_and_serve(li->data);
						if (0 == ret)
						{
							/* It's time to close the connection */
	 						fclose(li->data);
							/* We can't delete list element here, instead
							 * we mark it to delete later. */
							li->data = NULL;
						}
					}
				}
				/* Delete closed connections from list */
				fd_list = g_slist_remove_all(fd_list, NULL);
			}
		}
	}
	g_slist_foreach(fd_list, (GFunc) g_free, NULL);
	g_slist_free(fd_list);
	return 1;
}

int
main(int argc, char* argv[])
{
	/* socket stuff */
	int serversocket;
	struct sockaddr_un a;
	int ret;
	int len;
	char* socketname = "netdbsocket";
	
	serversocket = socket(PF_UNIX, SOCK_STREAM, 0);
	if (serversocket == -1)
	{
		perror("socket");
		return 1;
	}

	a.sun_family = AF_UNIX;
	len = sizeof(a.sun_path);
	strncpy(a.sun_path, socketname, len);
	
	ret = bind(serversocket, (struct sockaddr *) &a, len);
	if (ret == -1)
	{
		perror("bind");
		return 1;
	}
	
	ret = listen(serversocket, 3);
	if (ret == -1)
	{
		perror("listen");
		return 1;
	}

	serve_connections(serversocket);
	
	/* Clean up */
	close(serversocket);
	
	ret = unlink(socketname);
	if (ret == -1)
	{
		perror("unlink");
		return 1;
	}

	return 0;
}
