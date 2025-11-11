#include <string.h>
#include <stdlib.h>
#include "glob_vector.h"
#include "glob.h"

int handle_empty_pat(char *dir, t_list **lastlink,
		     t_list **firstmalloc, int *nalloca,
		     unsigned int *count)
{
	t_list *nextlink;
	char *nextname;

	if (glob_testdir(dir, 0) < 0)
		return -1;
	nextlink = (t_list *) alloca_safe(sizeof(struct globval));
	if (nextlink == NULL)
		return -1;
	nextlink->next = NULL;
	nextname = malloc(1);
	if (nextname == NULL)
		return -1;
	*lastlink = nextlink;
	nextlink->name = nextname;
	nextname[0] = '\0';
	*count = 1;
	return 0;
}

int handle_literal_pat(char *pat, size_t patlen, char *dir,
		       t_list **lastlink, t_list **firstmalloc,
		       int *nalloca, unsigned int *count)
{
	size_t dirlen;
	char *nextname;
	char *npat;
	t_list *nextlink;

	if (glob_testdir(dir, 0) < 0)
		return -1;
	dirlen = strlen(dir);
	nextname = malloc(dirlen + patlen + 2);
	npat = malloc(patlen + 1);
	if (nextname == NULL || npat == NULL)
	{
		free(nextname);
		free(npat);
		return -1;
	}
	strcpy(npat, pat);
	dequote_pathname(npat);
	strcpy(nextname, dir);
	nextname[dirlen++] = '/';
	strcpy(nextname + dirlen, npat);
	if (GLOB_TESTNAME(nextname) >= 0)
	{
		free(nextname);
		nextlink = (t_list *) alloca_safe(sizeof(struct globval));
		if (nextlink)
		{
			nextlink->next = NULL;
			*lastlink = nextlink;
			nextlink->name = npat;
			*count = 1;
			return 0;
		}
		free(npat);
		return -1;
	}
	free(nextname);
	free(npat);
	return 1;
}
