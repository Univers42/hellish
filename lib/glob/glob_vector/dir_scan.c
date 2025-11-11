#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "glob_vector.h"
#include "glob.h"

static int open_dir_stream(DIR **out, char *dir)
{
	*out = opendir(dir);
	if (*out == NULL)
		return -1;
	return 0;
}

static int prepare_convpat(char **out, char *pat)
{
	size_t patlen;

	patlen = pat ? strlen(pat) : 0;
	*out = fnx_fromfs(pat, patlen);
	if (*out != pat)
		*out = savestring(*out);
	return 0;
}

int scan_directory_and_collect(char *pat, char *dir, int flags,
			       t_list **lastlink, t_list **firstmalloc,
			       int *nalloca, unsigned int *count,
			       int *lose, int *add_current)
{
	DIR *d;
	struct dirent *dp;
	char *convpat;
	int mflags;
	int pflags;
	int isdir;
	char *subdir;
	char *convfn;

	if (open_dir_stream(&d, dir) < 0)
		return -1;
	prepare_convpat(&convpat, pat);
	if (noglob_dot_filenames)
		mflags = STRMATCH_PERIOD | STRMATCH_PNAME;
	else
		mflags = STRMATCH_DOTDOT | STRMATCH_PNAME;
#ifdef FNM_CASEFOLD
	if (glob_ignore_case)
		mflags |= STRMATCH_CASEFOLD;
#endif
	if (extended_glob)
		mflags |= STRMATCH_EXTMATCH;
	*add_current = ((flags & (GX_ALLDIRS | GX_ADDCURDIR)) ==
		       (GX_ALLDIRS | GX_ADDCURDIR));
	while (1)
	{
		if (interrupt_state || terminating_signal)
		{
			*lose = 1;
			break;
		}
		else if (signal_is_pending(SIGINT))
		{
			*lose = 1;
			break;
		}
		dp = readdir(d);
		if (dp == NULL)
			break;
		if (!REAL_DIR_ENTRY(dp))
			continue;
		if (skipname(pat, dp->d_name, flags))
			continue;
		isdir = -1;
		subdir = NULL;
		if (flags & (GX_MATCHDIRS | GX_ALLDIRS))
		{
			pflags = (flags & GX_ALLDIRS) ? MP_RMDOT : 0;
			if (flags & GX_NULLDIR)
				pflags |= MP_IGNDOT;
			subdir = sh_makepath(dir, dp->d_name, pflags);
			isdir = glob_testdir(subdir, flags);
			if (isdir < 0 && (flags & GX_MATCHDIRS))
			{
				free(subdir);
				continue;
			}
		}
		if (flags & GX_ALLDIRS)
		{
			if (isdir == 0)
			{
				t_list *e = NULL;
				int ndirs = 0;
				t_list *dirlist;

				dirlist = finddirs(pat, subdir, (flags & ~GX_ADDCURDIR),
						   &e, &ndirs);
				if (dirlist == &finddirs_error_return)
				{
					free(subdir);
					*lose = 1;
					break;
				}
				if (ndirs)
				{
					if (*firstmalloc == NULL)
						*firstmalloc = e;
					e->next = *lastlink;
					*lastlink = dirlist;
					*count += ndirs;
				}
			}
			{
				t_list *nextlink;
				size_t sdlen;
				char *nextname;

				nextlink = (t_list *) malloc(sizeof(struct globval));
				if (*firstmalloc == NULL)
					*firstmalloc = nextlink;
				sdlen = strlen(subdir);
				nextname = (char *) malloc(sdlen + 1);
				if (nextlink == NULL || nextname == NULL)
				{
					if (*firstmalloc && *firstmalloc == nextlink)
						*firstmalloc = NULL;
					free(nextlink);
					free(nextname);
					free(subdir);
					*lose = 1;
					break;
				}
				nextlink->next = *lastlink;
				*lastlink = nextlink;
				nextlink->name = nextname;
				memcpy(nextname, subdir, sdlen + 1);
				free(subdir);
				++(*count);
				continue;
			}
		}
		else if (flags & GX_MATCHDIRS)
		{
			free(subdir);
		}
		convfn = fnx_fromfs(dp->d_name, D_NAMLEN(dp));
		if (strmatch(convpat, convfn, mflags) != STRMATCH_NO)
		{
			t_list *nextlink;
			char *nextname;

			if (*nalloca < ALLOCA_MAX)
			{
				nextlink = (t_list *) alloca(sizeof(struct globval));
				*nalloca += sizeof(struct globval);
			}
			else
			{
				nextlink = (t_list *) malloc(sizeof(struct globval));
				if (*firstmalloc == NULL)
					*firstmalloc = nextlink;
			}
			nextname = (char *) malloc(D_NAMLEN(dp) + 1);
			if (nextlink == NULL || nextname == NULL)
			{
				if (*firstmalloc)
				{
					if (*firstmalloc == nextlink)
						*firstmalloc = NULL;
					free(nextlink);
				}
				free(nextname);
				*lose = 1;
				break;
			}
			nextlink->next = *lastlink;
			*lastlink = nextlink;
			nextlink->name = nextname;
			memcpy(nextname, dp->d_name, D_NAMLEN(dp) + 1);
			++(*count);
		}
	}
	closedir(d);
	if (convpat != pat)
		free(convpat);
	return 0;
}
