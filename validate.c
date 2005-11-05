#include <stdio.h>		/* sscanf */
#include <glib.h>		/* g_strdup_printf */
#include <sys/types.h> /* u_char */
#include <ctype.h> /* isspace */
#include <stdlib.h> /* malloc, strtol, free */

/* Given ip string returns standardized ip address as a string,
 * or NULL if address is bad. */
char *
get_std_ip_str (const char *buf)
{
  int a1 = 0, a2 = 0, a3 = 0, a4 = 0;
  char x;

  if (sscanf (buf, "%d.%d.%d.%d%c", &a1, &a2, &a3, &a4, &x) != 4)
    return NULL;
  if (a1 < 0 || a1 > 255)
    return NULL;
  if (a2 < 0 || a2 > 255)
    return NULL;
  if (a3 < 0 || a3 > 255)
    return NULL;
  if (a4 < 0 || a4 > 255)
    return NULL;
  return g_strdup_printf ("%d.%d.%d.%d", a1, a2, a3, a4);
}

/* Given mac address string returns standardized mac address
 * as a string, or NULL if address is bad. */
char *
get_std_mac_str (const char *buf)
{
  int a1 = 0, a2 = 0, a3 = 0, a4 = 0, a5 = 0, a6 = 0;
  char x;

  if (sscanf (buf, "%x:%x:%x:%x:%x:%x%c",
	      &a1, &a2, &a3, &a4, &a5, &a6, &x) != 6)
    return NULL;
  if (a1 < 0 || a1 > 255)
    return NULL;
  if (a2 < 0 || a2 > 255)
    return NULL;
  if (a3 < 0 || a3 > 255)
    return NULL;
  if (a4 < 0 || a4 > 255)
    return NULL;
  if (a5 < 0 || a5 > 255)
    return NULL;
  if (a6 < 0 || a6 > 255)
    return NULL;
  return g_strdup_printf ("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
			  a1, a2, a3, a4, a5, a6);
}

char *
mac_ntoa (u_char * mac)
{
  static char text[2 * 6 + 5 + 1];

  sprintf (text, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
	   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return text;
}

/* libnet_hex_aton from libnet 1.1.2.1 */
u_char *
libnet_hex_aton(char *s, int *len)
{
  u_char *buf;
  int i;
  int32_t l;
  char *pp;
      
  while (isspace(*s))
  {
    s++;
  }
  for (i = 0, *len = 0; s[i]; i++)
  {
    if (s[i] == ':')
    {
      (*len)++;
    }
  }
  buf = malloc(*len + 1);
  if (buf == NULL)
  {
    return (NULL);
  }
  /* expect len hex octets separated by ':' */
  for (i = 0; i < *len + 1; i++)
  {
    l = strtol(s, (char **)&pp, 16);
    if (pp == s || l > 0xff || l < 0)
    {
      *len = 0;
      free(buf);
      return (NULL);
    }
    if (!(*pp == ':' || (i == *len && (isspace(*pp) || *pp == '\0'))))
    {
      *len = 0;
      free(buf);
      return (NULL);
    }
    buf[i] = (u_char)l;
    s = pp + 1;
  }
  /* return int8_tacter after the octets ala strtol(3) */
  (*len)++;
  return (buf);
}

