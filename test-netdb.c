#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#define PNAME "test-netdb"

#include "ndb-client.h"

/* Executes getmac command on netdb count times */
void test_netdb(int count)
{
	int ret;
	char *msg;
	int i;
	
	for (i = 0; i < count; i++)
	{
		ret = execute_command(&msg, "getmac", "127.0.0.1");
		if (1 != ret)
		{
			fprintf(stderr, "execute_command: %s\n", msg);
		}
		g_free(msg);
	}
	
	return;
}

int main(int argc, char *argv[])
{
	int ret;
	char *msg;
	if (argc < 3)
	{
		printf("Usage:\n\t%s socketname iteration_count\n", PNAME);
		return 1;
	}
	ret = ndb_init(argv[1], &msg);
	if (-1 == ret)
	{
		fprintf(stderr, "ndb_init: %s\n", msg);
		return 1;
	}

	/* Let's test netdb */
	test_netdb(atoi(argv[2]));
	
	ndb_cleanup();
	return 1;
}
