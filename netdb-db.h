#ifndef _NETDB_DB_H_
#define _NETDB_DB_H_

int db_add(char *ip, char *mac);
int db_remove(char *ip);
void db_init();
void db_free();
char *db_getmac(char *ip);
char *db_dump(char *format_string);

int db_setvar(char *variable, char *value);
char *db_getvar(char *variable);

#endif
