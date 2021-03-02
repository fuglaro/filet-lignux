/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc failed.\n");
	return p;
}

void
die(const char *msg) {
	fputs(msg, stderr);
	exit(1);
}
