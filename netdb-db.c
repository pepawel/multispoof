#include <glib.h>

/* Global hash table for storing ip-mac info, indexed by ip.
 * Ip and mac stored as strings. */
GHashTable *db;

/* Allocate memory for hash table. */
void
db_init()
{
	/* Create hash table and specify functions called at destroing
	 * elements to free them. */
	db = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	return;
}

/* Free hash table. */
void
db_free()
{
	/* There is no need to free elements, because this function will
	 * do so automatically (see comment in db_init) */
	g_hash_table_destroy(db);
	return;
}

/* Adds given ip and mac pair if ip doesn't exist in hash table yet.
 * If ip already exists returns -1, otherwise returns 1. */
int
db_add(char* ip, char* mac)
{
	/* Check if entry for this ip already exists */
	if (NULL == g_hash_table_lookup(db, ip))
	{
		g_hash_table_insert(db, g_strdup(ip), g_strdup(mac));
		return 1;
	}
	else
		return -1;
}

/* Removes given ip from db. Return 1 if ip was found and removed,
 * -1 otherwise. */
int
db_remove(char* ip)
{
	gboolean ret = g_hash_table_remove(db, ip);
	return (ret ? 1 : -1);
}

/* Returns mac for a given ip, or NULL if not found.
 * Returned string points to global hash table and
 * should not be freed. */
char *
db_getmac(char* ip)
{
	return g_hash_table_lookup(db, ip);
}
	
/* Internal struct used by db_dump */
typedef struct
{
	char *str; /* String which is appended during dump operation */
	char *format_string; /* Format string for entry formatting */
} _db_dump_str_t;

/* Internal function to append entries to dump string */
static void
_db_append(gpointer ipp, gpointer macp, gpointer dumpp)
{
	char *ip = (char *) ipp;
	char *mac = (char *) macp;
	_db_dump_str_t *dump = (_db_dump_str_t *) dumpp;
	char *line;
	
	line = g_strdup_printf(dump->format_string, ip, mac);
	dump->str = g_strconcat(dump->str, line, NULL);
	g_free(line);
	
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
db_dump(char *format_string)
{
	_db_dump_str_t dump;
	dump.str = g_strdup("");
	dump.format_string = g_strdup(format_string);
	
	g_hash_table_foreach(db, _db_append, &dump);

	g_free(dump.format_string);
	return dump.str;
}
