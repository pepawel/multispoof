#ifndef _NETDB_DB_H_
#define _NETDB_DB_H_

#include <sys/time.h>
#include <time.h>

int db_add_replace_update (char *ip, char *mac);
int db_change_enabled (char *ip, int val);
int db_start_stop_test (char *ip, int val);
int db_remove (char *ip);
void db_init ();
void db_free ();
int db_gethost (char **out_mac, time_t * out_age, time_t * out_test_age,
		int *out_enabled, int *out_cur_test, char *ip);
char *db_dump ();

int db_setvar (char *variable, char *value);
char *db_getvar (char *variable);

#endif
