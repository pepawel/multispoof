#ifndef _NDB_CLIENT_H_
#define _NDB_CLIENT_H_

#include <libnet.h>		/* u_int8_t FIXME: remove dependancy on libnet */

int execute_command (char **out_buf, char *command, char *arg);
int execute_command_long (char ***out_vector, char *command, char *arg);
int ndb_execute_gethost (u_int8_t * mac, int *out_enabled, struct in_addr ip);
int ndb_init (char *socketname, char **out_error);
void ndb_cleanup ();

#endif
