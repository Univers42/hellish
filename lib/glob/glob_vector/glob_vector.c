#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "glob_vector.h"
#include "glob.h"

char **glob_vector(char *pat, char *dir, int flags)
{
	t_list *lastlink = NULL;
	unsigned int count = 0;
	int lose = 0;
	int skip = 0;
	size_t patlen = 0;
	char **name_vector = NULL;
	int hasglob = 0;
	int nalloca = 0;
	t_list *firstmalloc = NULL;
	char *nextname = NULL;
	int addcur = 0;
	int r;

	if (pat == NULL || *pat == '\0')
	{
		if (handle_empty_pat(dir, &lastlink, &firstmalloc, &nalloca, &count) < 0)
			return (char **) &glob_error_return;
		skip = 1;
	}
	patlen = (pat && *pat) ? strlen(pat) : 0;
	if (!skip && ((hasglob = glob_pattern_p(pat)) == 0 || hasglob == 2))
	{
		int h = handle_literal_pat(pat, patlen, dir, &lastlink,
					   &firstmalloc, &nalloca, &count);
		if (h < 0)
			return (char **) &glob_error_return;
		if (h == 0)
			skip = 1;
	}
	if (!skip)
	{
		addcur = 0;
		r = scan_directory_and_collect(pat, dir, flags, &lastlink, &firstmalloc,
					       &nalloca, &count, &lose, &addcur);
		if (r < 0)
			return (char **) &glob_error_return;
		if (addcur)
			;
	}
	if ((flags & (GX_ALLDIRS | GX_ADDCURDIR)) == (GX_ALLDIRS | GX_ADDCURDIR))
		addcur = 1;
	if (addcur && !lose)
	{
		size_t sdlen = strlen(dir);
		nextname = (char *) malloc(sdlen + 1);
		t_list *nextlink = (t_list *) malloc(sizeof(struct globval));
		if (nextlink == NULL || nextname == NULL)
		{
			free(nextlink);
			free(nextname);
			lose = 1;
		}
		else
		{
			nextlink->name = nextname;
			nextlink->next = lastlink;
			lastlink = nextlink;
			if (flags & GX_NULLDIR)
				nextname[0] = '\0';
			else
				memcpy(nextname, dir, sdlen + 1);
			++count;
		}
	}
	if (!lose)
	{
		name_vector = (char **) malloc((count + 1) * sizeof(char *));
		if (name_vector == NULL)
			lose = 1;
	}
	if (lose)
	{
		free_globvals_on_error(lastlink, firstmalloc);
		return NULL;
	}
	{
		t_list *tmplink = lastlink;
		unsigned int i = 0;
		while (i < count && tmplink)
		{
			name_vector[i++] = tmplink->name;
			tmplink = tmplink->next;
		}
		name_vector[count] = NULL;
	}
	if (firstmalloc)
		free_alloced_nodes(lastlink, firstmalloc);
	return name_vector;
}
