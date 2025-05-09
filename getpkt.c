#include <stdio.h>
#include "getpkt.h"

#include "common.h"

/* Convert hexadecimal string to array of bytes tab of size length.
 * Returns 1 on success, -1 otherwise. */
int
hex2byte (tab, string, size)
     u_char *tab;
     char *string;
     u_int16_t *size;
{
  int i;
  char c;
  int d;

  c = 'c';
  for (i = 0;; i++)
  {
    c = string[i];
    if ((c == '\0') || (c == '\n'))
      break;
    if ((c >= '0') && (c <= '9'))
      d = c - '0';
    else if ((c >= 'a') && (c <= 'f'))
      d = c - 'a' + 10;
    else if ((c >= 'A') && (c <= 'F'))
      d = c - 'A' + 10;
    else
    {
      return -1;
    }
    if ((i % 2) == 0)
      tab[i / 2] = d * 16;
    else
      tab[i / 2] += d;
  }
  if ((i - 1) % 2)
  {
    *size = i / 2;
    return 1;
  }
  else
    return -1;
}

/* Gets packet in hexadecimal line form from standard input.
 * p - buffer for the packet
 * s - actual size of this packet
 * Returns 1 on success, 0 on eof and -1 on error.
 */
int
get_packet (p, s)
     u_char *p;
     u_int16_t *s;
{
  /* Textual representation of a packet + new line sign */
  static char tbuf[TBUF_SIZE];
  int ret, result = -1;
  char *pret;

  /* Get textual data from stdin */
  pret = fgets (tbuf, TBUF_SIZE, stdin);
  if (NULL == pret)
  {
    if (feof (stdin))
      result = 0;
    else
      result = -1;
  }
  else
  {
    /* Convert textual representation into binary representation */
    ret = hex2byte (p, tbuf, s);
    if (-1 == ret)
    {
      result = -1;
    }
    else
      result = 1;
  }
  return result;
}
