#ifndef _NETDB_DB_H_
#define _NETDB_DB_H_

#include <sys/time.h>
#include <time.h>

int db_add_replace_update (char *ip, char *mac);
int db_change_enabled (char *ip, int val);
int db_remove (char *ip);
void db_init ();
void db_free ();
int db_gethost (char **out_mac, int *out_enabled, int *age, char *ip);
char *db_dump (char *format_string);
char *db_listenabled (int age);

int db_setvar (char *variable, char *value);
char *db_getvar (char *variable);

#endif
