#ifndef _NDB_CLIENT_H_
#define _NDB_CLIENT_H_

int execute_command (char **out_buf, char *command, char *arg);
int ndb_init (char *socketname, char **out_error);
void ndb_cleanup ();

#endif
