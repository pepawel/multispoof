#include <glib.h>
#include <string.h>		/* strcmp */
#include <sys/time.h> /* gettimeofday */
#include <time.h> /* gettimeofday */
#include <stdlib.h> /* exit */

/* Global hash table for storing ip-mac info, indexed by ip.
 * Ip and mac stored as strings. */
GHashTable *db_mac;

/* Same as above, but for last seen time. Time is stored
 * as integer (you need to call GPOINTER_TO_INT or GINT_TO_GPOINTER
 * macros to access it) */
GHashTable *db_time;

/* Variables support */
typedef struct
{
  char *name;
  char *value;
} variable_t;

/* Global array of variables */
variable_t variable_tab[] = {
  {"defmac", (char *) NULL},
  {(char *) NULL, (char *) NULL}
};

/* Returns current time in seconds. */
int
getcurtime ()
{
  struct timeval tv;
  struct timezone tz;
  if (-1 == gettimeofday (&tv, &tz))
    exit(EXIT_FAILURE);
  return tv.tv_sec;
}


/* Allocate memory for hash tables. */
void
db_init ()
{
  /* Create hash tables and specify functions called at destroing
   * elements to free them. */
  db_mac = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  db_time = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  return;
}

/* Free hash table and variables. */
void
db_free ()
{
  int i;

  /* There is no need to free elements, because this function will
   * do so automatically (see comment in db_init) */
  g_hash_table_destroy (db_mac);
  g_hash_table_destroy (db_time);

  /* Free variables */
  for (i = 0; NULL != variable_tab[i].name; i++)
  {
    g_free (variable_tab[i].value);	/* safe: if NULL nothing happens */
  }
  return;
}

/* If ip doesn't exist in hash - adds new entry.
 * If ip do exist, but registered mac is different - updates mac.
 * If ip do exist and new mac is identical to old one - does nothing.
 * 
 * In any of these cases updates last seen time of ip.
 * 
 * Return value:
 * 1 if entry was added,
 * 2 if mac was updated,
 * 3 if only last seen time was updated.
 */
int
db_add_replace_update (char *ip, char *mac)
{
  int result = -1;
  char *old_mac = g_hash_table_lookup (db_mac, ip);

  /* Check if entry for this ip already exists */
  if (NULL == old_mac)
  {
    /* Adding new entry */
    g_hash_table_insert (db_mac, g_strdup (ip), g_strdup (mac));
    result = 1;
  }
  else if (0 != strcmp (old_mac, mac))
  {
    /* Modyfing existing entry */
    g_hash_table_replace (db_mac, g_strdup (ip), g_strdup (mac));
    result = 2;
  }
  else
  {
    /* Updating last seen time only */
    result = 3;
  }

  /* Update last seen time - FIXME */
  g_hash_table_replace (db_time, g_strdup (ip),
			GINT_TO_POINTER (getcurtime ()));

  return result;
}

/* Removes given ip from db. Return 1 if ip was found and removed,
 * -1 otherwise. */
int
db_remove (char *ip)
{
  gboolean ret1 = g_hash_table_remove (db_mac, ip);
  gboolean ret2 = g_hash_table_remove (db_time, ip);

  return ((ret1 && ret2) ? 1 : -1);
}

/* Returns mac for a given ip, or NULL if not found.
 * Returned string points to global hash table and
 * should not be freed. */
char *
db_getmac (char *ip)
{
  return g_hash_table_lookup (db_mac, ip);
}

/* Returns last seen time for a given ip, or -1 if not found. */
int
db_gettime (char *ip)
{
  gpointer tmp, time;
  int ret = g_hash_table_lookup_extended (db_time, ip, &tmp, &time);
  if (0 != ret)
    return getcurtime() - GPOINTER_TO_INT(time);
  else
    return -1;
}

/* Internal struct used by db_dump */
typedef struct
{
  char *str;			/* String which is appended during dump operation */
  char *format_string;		/* Format string for entry formatting */
} _db_dump_str_t;

/* Internal function to append entries to dump string */
static void
_db_append (gpointer ipp, gpointer macp, gpointer dumpp)
{
  char *ip = (char *) ipp;
  char *mac = (char *) macp;
  _db_dump_str_t *dump = (_db_dump_str_t *) dumpp;
  char *line;

  line = g_strdup_printf (dump->format_string, ip, mac);
  dump->str = g_strconcat (dump->str, line, NULL);
  g_free (line);

  return;
}

/* Function returns newly allocated string which is generated
 * using format_string for every entry in db. After use this
 * string should be freed.
 * When db is empty, then it returns empty string (need to be
 * freed anyway).
 *
 * Format string can use two strings: ip and mac (in that order)
 * 
 * Warning: you should take care to not allow user to pass
 * arbitrary format_strings to prevent format string attacks. */
char *
db_dump (char *format_string)
{
  _db_dump_str_t dump;

  dump.str = g_strdup ("");
  dump.format_string = g_strdup (format_string);

  g_hash_table_foreach (db_mac, _db_append, &dump);

  g_free (dump.format_string);
  return dump.str;
}

/* Associates value with variable. Allocated memory for value.
 * Returns 1 on success, -1 when variable not found.
 * NOTE: You can set only allowed variables - listed in
 *       variable_tab[] global array */
int
db_setvar (char *variable, char *value)
{
  int i;

  for (i = 0; NULL != variable_tab[i].name; i++)
  {
    if (0 == strcmp (variable, variable_tab[i].name))
    {
      g_free (variable_tab[i].value);	/* safe: if NULL nothing happens */
      variable_tab[i].value = g_strdup (value);
      return 1;
    }
  }
  return -1;
}

/* Returns pointer to variable value. Returns value or NULL
 * if variable not found or not set. */
char *
db_getvar (char *variable)
{
  int i;

  for (i = 0; NULL != variable_tab[i].name; i++)
  {
    if (0 == strcmp (variable, variable_tab[i].name))
      return variable_tab[i].value;
  }
  return NULL;
}
