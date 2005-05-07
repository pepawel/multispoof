#include <glib.h>
#include <string.h>		/* strcmp */
#include <time.h>		/* time */
#include <stdlib.h>		/* exit */

/* Global hash table for storing host info, indexed by ip.
 * Ip stored as string. */
GHashTable *db;

/* Host entry in hash table */
#define MAC_TEXT_SIZE 6*2+5+1
typedef struct
{
  char mac[MAC_TEXT_SIZE];	/* mac as ":"-separated string */
  time_t last_active;		/* When the host was last active in seconds */
  int enabled;			/* Flag set if host is ready to be used */
} host_entry_t;

/* Variables support */
typedef struct
{
  char *name;
  char *value;
} variable_t;

/* Global array of variables */
variable_t variable_tab[] = {
  {"defmac", (char *) NULL},	/* Default mac address for cmac unspoof */
  {(char *) NULL, (char *) NULL}
};

/* Allocate memory for hash table. */
void
db_init ()
{
  /* Create hash table and specify functions called at destroing
   * elements to free them. */
  db = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  return;
}

/* Free hash table and variables. */
void
db_free ()
{
  int i;

  /* There is no need to free elements, because this function will
   * do so automatically (see comment in db_init) */
  g_hash_table_destroy (db);

  /* Free variables */
  for (i = 0; NULL != variable_tab[i].name; i++)
  {
    g_free (variable_tab[i].value);	/* safe: if NULL nothing happens */
  }
  return;
}

/* Allocates memory for new host entry and fills fields with defaults.
 * Returns host entry. */
host_entry_t *
_new_host_entry (char *mac, time_t time, int enabled)
{
  host_entry_t *e;

  e = g_malloc (sizeof (host_entry_t));
  g_strlcpy (e->mac, mac, MAC_TEXT_SIZE);
  e->last_active = time;
  e->enabled = enabled;
  return e;
}

/* If ip doesn't exist in hash - adds new entry.
 * If ip do exist, but registered mac is different - updates mac.
 * If ip do exist and new mac is identical to old one - does nothing.
 * 
 * In any of these cases updates last active time of ip.
 * 
 * Return value:
 * 1 if entry was added,
 * 2 if mac was updated,
 * 3 if only last active time was updated.
 */
int
db_add_replace_update (char *ip, char *mac)
{
  int result = -1;
  host_entry_t *old_entry = g_hash_table_lookup (db, ip);

  /* Check if entry for this ip already exists */
  if (NULL == old_entry)
  {
    /* Adding new entry */
    g_hash_table_insert (db, g_strdup (ip),
			 _new_host_entry (mac, time (NULL), 0));
    result = 1;
  }
  else if (0 != strcmp (old_entry->mac, mac))
  {
    /* Modyfing existing entry */
    g_hash_table_replace (db, g_strdup (ip),
			  _new_host_entry (mac, time (NULL), 0));
    old_entry->last_active = time (NULL);
    result = 2;
  }
  else
  {
    /* Updating last seen time only */
    old_entry->last_active = time (NULL);
    result = 3;
  }

  return result;
}

/* Updates enabled flag in host entry for given ip with given value.
 * Return -1 on error, >=1 otherwise. */
int
db_change_enabled (char *ip, int val)
{
  host_entry_t *e = g_hash_table_lookup (db, ip);

  if (NULL == e)
  {
    return -1;
  }
  else if (val != e->enabled)
  {
    e->enabled = val;
    return 1;
  }
  else
  {
    return 2;
  }
}

/* Removes given ip from db. Return 1 if ip was found and removed,
 * -1 otherwise. */
int
db_remove (char *ip)
{
  gboolean ret = g_hash_table_remove (db, ip);

  return ret ? 1 : -1;
}

/* Returns mac for a given ip, or NULL if not found.
 * Returned string points to global hash table and
 * should not be freed. */
char *
db_getmac (char *ip)
{
  host_entry_t *e;

  e = g_hash_table_lookup (db, ip);
  return e != NULL ? e->mac : NULL;
}

/* Returns inactivity age for a given ip, or -1 if not found. */
time_t
db_getage (char *ip)
{
  host_entry_t *e;

  e = g_hash_table_lookup (db, ip);
  return e != NULL ? (time (NULL) - e->last_active) : -1;
}

/* Internal struct used by db_dump */
typedef struct
{
  char *str;			/* String which is appended during dump operation */
  char *format_string;		/* Format string for entry formatting */
} _db_dump_str_t;

/* Internal function to append entries to dump string */
static void
_db_dump_append (gpointer ipp, gpointer entryp, gpointer dumpp)
{
  char *ip = (char *) ipp;
  host_entry_t *e = (host_entry_t *) entryp;
  _db_dump_str_t *dump = (_db_dump_str_t *) dumpp;
  char *line;

  line = g_strdup_printf (dump->format_string, ip, e->mac);
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

  g_hash_table_foreach (db, _db_dump_append, &dump);

  g_free (dump.format_string);
  return dump.str;
}

/* Internal struct used by db_listenabled */
typedef struct
{
  char *str;			/* String which is appended during dump operation */
  time_t age;			/* Age of entry */
} _db_listenabled_str_t;

/* Internal function to append entries to dump string */
static void
_db_listenabled_append (gpointer ipp, gpointer entryp, gpointer dumpp)
{
  char *ip = (char *) ipp;
  host_entry_t *e = (host_entry_t *) entryp;
  _db_listenabled_str_t *dump = (_db_listenabled_str_t *) dumpp;

  if ((1 == e->enabled) && ((time (NULL) - e->last_active) >= dump->age))
  {
    dump->str = g_strconcat (dump->str, ip, "\n", NULL);
  }

  return;
}

/* Function returns newly allocated string which is generated
 * for every entry in db which age is >= than given value. After use
 * this string should be freed.
 * When db is empty, then it returns empty string (need to be
 * freed anyway). */
char *
db_listenabled (int age)
{
  _db_listenabled_str_t dump;

  dump.age = age;
  dump.str = g_strdup ("");
  g_hash_table_foreach (db, _db_listenabled_append, &dump);
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
