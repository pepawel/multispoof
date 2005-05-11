#ifndef _NDB_CLIENT_H_
#define _NDB_CLIENT_H_

#define MSG_BUF_SIZE 128

#include <libnet.h>		/* u_int8_t FIXME: remove dependancy on libnet */

int execute_command (char *msg_buf, char *command, char *arg);
int execute_command_long (char ***out_vector, char *command, char *arg);
int ndb_execute_gethost (u_int8_t * mac, time_t * out_age,
			 time_t * out_test_age, int *out_enabled,
			 int *out_cur_test, struct in_addr ip);
int ndb_execute_host (struct in_addr ip, u_int8_t * mac);
int fetch_host_tab (struct in_addr **out_tab, int *out_count);
int ndb_init (char *socketname, char **out_error);
void ndb_cleanup ();

#endif
