#ifndef _NETDB_H_
#define _NETDB_H_

#include <glib.h>

typedef int netdb_func_t (gchar **, guint, gchar **);

typedef struct
{
  gchar *name;
  netdb_func_t *func;
} command_t;

extern command_t commands[];

#endif
