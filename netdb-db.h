#ifndef _NETDB_DB_H_
#define _NETDB_DB_H_

int db_add(char *ip, char *mac);
void db_init();
void db_free();
char *db_getmac(char *ip);
char *db_dump(char *format_string);

#endif
