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
  time_t last_tested;		/* When the host was last tested in seconds */
  int enabled;			/* Flag set if host is ready to be used */
  int cur_test;			/* Flag set if host is currently tested */
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
  {"banned", (char *) NULL},	/* IP addresses which can't be added */
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
_new_host_entry (mac, a_time, t_time, enabled, cur_test)
     char *mac;
     time_t a_time;
     time_t t_time;
     int enabled;
     int cur_test;
{
  host_entry_t *e;

  e = g_malloc (sizeof (host_entry_t));
  g_strlcpy (e->mac, mac, MAC_TEXT_SIZE);
  e->last_active = a_time;
  e->last_tested = t_time;
  e->enabled = enabled;
  e->cur_test = cur_test;
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
			 _new_host_entry (mac, time (NULL), (time_t) 0, 0,
					  0));
    result = 1;
  }
  else if (0 != strcmp (old_entry->mac, mac))
  {
    /* Modyfing existing entry */
    g_strlcpy (old_entry->mac, mac, MAC_TEXT_SIZE);
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

/* Updates enabled cur_test flag in host entry for given ip with
 * given value. When val = 1 it means start of test.
 * When val = 0 it stops test and fills last_tested with current time.
 * Return -1 on error, 1 otherwise. */
int
db_start_stop_test (char *ip, int val)
{
  host_entry_t *e = g_hash_table_lookup (db, ip);

  if (NULL == e)
  {
    return -1;
  }
  else
  {
    e->cur_test = val;
    if (0 == val)
      e->last_tested = time (NULL);
    return 1;
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

/* Given ip finds host entry, fills mac and enabled variabled
 * and returns 1. Returns 0 if entry was not found. */
int
db_gethost (out_mac, out_age, out_test_age, out_enabled, out_cur_test, ip)
     char **out_mac;
     time_t *out_age;
     time_t *out_test_age;
     int *out_enabled;
     int *out_cur_test;
     char *ip;
{
  host_entry_t *e;
  time_t now = time (NULL);

  e = g_hash_table_lookup (db, ip);
  if (NULL != e)
  {
    *out_mac = e->mac;
    *out_age = now - e->last_active;
    *out_test_age = now - e->last_tested;
    *out_enabled = e->enabled;
    *out_cur_test = e->cur_test;
    return 1;
  }
  else
    return -1;
}

/* Internal function used by db_dump(). */
static void
_db_dump_append (gpointer ipp, gpointer entryp, gpointer dumpp)
{
  char *new_dump;

  new_dump = g_strconcat (*((char **) dumpp), ipp, "\n", NULL);
  g_free (*((char **) dumpp));
  (*((char **) dumpp)) = new_dump;
  return;
}

/* Function returns newly allocated string which is new line
 * separated list of ips. After use this string should be freed.
 * When db is empty, then it returns empty string (need to be
 * freed anyway). */
char *
db_dump ()
{
  char *dump;

  dump = g_strdup ("");
  g_hash_table_foreach (db, _db_dump_append, &dump);
  return dump;
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
