/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>

#include "bt_utlis.h"
extern int hwdec_write_align(unsigned char type, unsigned char *data, int len);

char *u_strtok_r(char *str, const char *delim, char **saveptr)
{
  char *pbegin;
  char *pend = NULL;

  /* Decide if we are starting a new string or continuing from
   * the point we left off.
   */

  if (str)
    {
      pbegin = str;
    }
  else if (saveptr && *saveptr)
    {
      pbegin = *saveptr;
    }
  else
    {
      return NULL;
    }

  /* Find the beginning of the next token */

  for (;
       *pbegin && strchr(delim, *pbegin) != NULL;
       pbegin++);

  /* If we are at the end of the string with nothing
   * but delimiters found, then return NULL.
   */

  if (!*pbegin)
    {
      return NULL;
    }

  /* Find the end of the token */

  for (pend = pbegin + 1;
       *pend && strchr(delim, *pend) == NULL;
       pend++);


  /* pend either points to the end of the string or to
   * the first delimiter after the string.
   */

  if (*pend)
    {
      /* Turn the delimiter into a null terminator */

      *pend++ = '\0';
    }

  /* Save the pointer where we left off and return the
   * beginning of the token.
   */

  if (saveptr)
    {
      *saveptr = pend;
    }
  return pbegin;
}

int bt_sipc_send(unsigned char *data, int len)
{
	HCIDUMP("-> ", data, len);
	hwdec_write_align(data[0], &data[1], len-1);
	return len;
}
