#ifndef _NDB_CLIENT_H_
#define _NDB_CLIENT_H_

#include <sys/types.h> /* u_char */

/* in_addr */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSG_BUF_SIZE 128

int execute_command (char *msg_buf, char *command, char *arg);
int execute_command_long (char ***out_vector, char *command, char *arg);
int ndb_execute_gethost (u_char * mac, time_t * out_age,
			 time_t * out_test_age, int *out_enabled,
			 int *out_cur_test, struct in_addr ip);
int ndb_execute_host (struct in_addr ip, u_char * mac);
int ndb_execute_do (char *op, struct in_addr ip);
int fetch_host_tab (struct in_addr **out_tab, int *out_count);
int ndb_init (char *socketname, char **out_error);
void ndb_cleanup ();

#endif
