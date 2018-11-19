/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>

#include "bt_utlis.h"

void hexdump(char *tag, unsigned char *bin, size_t binsz)
{
	char str[500], hex_str[]= "0123456789ABCDEF";
	size_t i;

	for (i = 0; i < binsz; i++) {
		str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
		str[(i * 3) + 1] = hex_str[(bin[i]     ) & 0x0F];
		str[(i * 3) + 2] = ' ';
	}
	str[(binsz * 3) - 1] = 0x00;
	if (tag)
		printk("%s %s\n", tag, str);
	else
		printk("%s\n", str);
}

void hex_dump_block(char *tag, unsigned char *bin, size_t binsz)
{
	int loop = binsz / DUMP_BLOCK_SIZE;
	int tail = binsz % DUMP_BLOCK_SIZE;
	int i;

	if (!loop) {
		hexdump(tag, bin, binsz);
		return;
	}

	for (i = 0; i < loop; i++) {
		hexdump(tag, bin + i * DUMP_BLOCK_SIZE, DUMP_BLOCK_SIZE);
	}

	if (tail)
		hexdump(tag, bin + i * DUMP_BLOCK_SIZE, tail);
}

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
