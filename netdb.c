#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h> /* unlink, select */
#include <fcntl.h> /* fcntl */
#include <sys/time.h> /* select */

#define max(a,b) ((a)>(b) ? (a):(b))

int
main(int argc, char* argv[])
{
	/* socket stuff */
	int serversocket;
	int clientsocket;
	struct sockaddr_un a;
	int ret;
	int len;
	char* socketname = "netdbsocket";
	
	char buf[100];
	FILE *s;

	/* select stuff */
	fd_set fds;
	int max_fd;
	
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

	/* while(1) */
	{
		FD_ZERO(&fds);
		FD_SET(serversocket, &fds);
		max_fd = max(serversocket,0) + 1;

		select(max_fd, &fds, NULL, NULL, NULL);

		if (FD_ISSET(serversocket, &fds))
		{
			clientsocket = accept(serversocket, NULL, NULL);
			if (clientsocket == -1)
			{
				perror("accept");
				return 1;
			}

			s = fdopen(clientsocket, "r+");
			if (s == NULL)
			{
				perror("fdopen");
				return 1;
			}

			fprintf(s, "Welcome!\n");
			fgets(buf, 50, s);
			printf("We lurked from socket: %s\n", buf);
		}
	}
	/* Clean up */
	shutdown(fileno(s), 2); /* disable reads and writes */
	fclose(s);
	close(serversocket);
	
	ret = unlink(socketname);
	if (ret == -1)
	{
		perror("unlink");
		return 1;
	}

	return 0;
}
