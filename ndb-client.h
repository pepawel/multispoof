#ifndef _NDB_CLIENT_H_
#define _NDB_CLIENT_H_

int execute_command (char **out_buf, char *command, char *arg);
int ndb_execute_getmac (u_int8_t * mac, struct in_addr ip);
int ndb_init (char *socketname, char **out_error);
void ndb_cleanup ();

#endif
