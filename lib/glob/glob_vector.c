/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_vector.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 19:23:25 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 19:41:48 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define _POSIX_C_SOURCE 200809L
#include "glob_vector.h"
#include <string.h>
#include <stdio.h>
#include <alloca.h>
#include <unistd.h>
#include "glob.h"

/* Internal small helpers to keep each function short and readable. */

/* Allocate a block via alloca and return pointer or NULL on failure.
   Note: alloca failures are typically non-detectable; here we try to be
   safe by using malloc fallback when possible. */
static void *alloca_safe(size_t n)
{
	void	*p;

	if (n <= 1024)
		return alloca(n);
	p = malloc(n);
	return (p);
}

/* Free a globval chain nodes that were allocated with malloc.
   FIRSTMALLOC is the earliest node that was malloc'ed; nodes before that
   come from alloca and must not be free()'d. */
static void	free_globvals_on_error(t_list *lastlink, t_list *firstmalloc)
{
	t_list	*tmplink;

	while (lastlink)
	{
		if (firstmalloc)
		{
			if (lastlink == firstmalloc)
				firstmalloc = NULL;
			tmplink = lastlink;
		}
		else
			tmplink = NULL;
		if (lastlink->name)
			free(lastlink->name);
		lastlink = lastlink->next;
		if (tmplink)
			free(tmplink);
	}
}

/* Free only the struct nodes that were malloc'ed (firstmalloc..end). */
static void
free_alloced_nodes(t_list *lastlink, t_list *firstmalloc)
{
	t_list	*tmplink;
	t_list	*next;
	t_list	*tmp;

	if (firstmalloc == NULL)
		return;
	/* We must walk and free nodes that were malloced. The chain is in
	   reverse order: lastlink -> ... -> firstmalloc -> ... -> NULL. */
	tmplink = NULL;
	while (lastlink)
	{
		next = lastlink->next;
		if (lastlink == firstmalloc)
		{
			while (lastlink)
			{
				tmp = lastlink->next;
				free(lastlink);
				lastlink = tmp;
			}
			break ;
		}
		lastlink = next;
	}
}

/* Build a globval node on stack (alloca) or heap (malloc) according to
   current nalloca usage. Update firstmalloc when using malloc. */
static t_list *make_globval_node(t_list **firstmalloc, int *nalloca)
{
	t_list *node;

	if (*nalloca < ALLOCA_MAX)
	{
		node = (t_list *) alloca(sizeof(struct globval));
		*nalloca += sizeof(struct globval);
	}
	else
	{
		node = (t_list *) malloc(sizeof(struct globval));
		if (node && *firstmalloc == NULL)
			*firstmalloc = node;
	}
	return (node);
}

/* Helper to push a node containing NAME (already allocated) onto lastlink
   chain. NAME ownership transferred to node. */
static int	push_name_node(t_list **lastlink, t_list **firstmalloc,
		   int *nalloca, char *name)
{
	t_list *node;

	node = make_globval_node(firstmalloc, nalloca);
	if (node == NULL)
		return -1;
	node->next = *lastlink;
	*lastlink = node;
	node->name = name;
	return (0);
}

/* Try the empty-pattern special case: produce a single empty-name entry. */
static int	handle_empty_pat(char *dir, t_list **lastlink, t_list **firstmalloc,
		 int *nalloca, unsigned int *count)
{
	t_list	*nextlink;
	char	*nextname;

	if (glob_testdir(dir, 0) < 0)
		return (-1);
	nextlink = (t_list *) alloca_safe(sizeof(struct globval));
	if (nextlink == NULL)
		return (-1);
	nextlink->next = NULL;
	nextname = (char *) malloc(1);
	if (nextname == NULL)
		return (-1);
	lastlink[0] = nextlink;
	nextlink->name = nextname;
	nextname[0] = '\0';
	*count = 1;
	return (0);
}

/* Try non-glob literal pattern optimization (no special chars or only
   backslash escapes): check DIR/PAT existence and add a single node if exists. */
static int	handle_literal_pat(char *pat, size_t patlen, char *dir,
		   t_list **lastlink, t_list **firstmalloc,
		   int *nalloca, unsigned int *count)
{
	size_t			dirlen;
	char  			*nextname = NULL;
	char  			*npat = NULL;
	t_link			*nextlink;

	if (glob_testdir(dir, 0) < 0)
		return (-1);
	dirlen = strlen(dir);
	nextname = malloc(dirlen + patlen + 2);
	npat = malloc(patlen + 1);
	if (nextname == NULL || npat == NULL)
		return (free(nextname), free(npat), -1);
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
		else
			return (free(npat), -1);
	}
	return (free(nextname), free(npat), 1);
}

/* Main directory scan loop: open DIR, scan entries, build matching entries.
   Returns -1 on fatal error (map to glob_error_return), 0 on success,
   1 if caller should continue (no matches added here). */
static int	scan_directory_and_collect(char *pat, char *dir, int flags,
			   t_list **lastlink, t_list **firstmalloc,
			   int *nalloca, unsigned int *count, int *lose, int *add_current)
{
	DIR				*d;
	struct dirent	*dp;
	char			*convpat = NULL;
	int				mflags;
	int				pflags;
	int				isdir = -1;
	char			*subdir = NULL;
	char			*convfn = NULL;
	size_t			patlen;
	t_list			*e;
	int				ndirs;
	t_list			*dirlist;
	t_list *nextlink;
	size_t sdlen;
	char	*nextname;
	
	d = opendir(dir);
	if (d == NULL)
		return -1;
	if (pat)
		patlen = strlen(pat);
	else
		patlen = 0;
	convpat = fnx_fromfs(pat, patlen);
	if (convpat != pat)
		convpat = savestring(convpat);
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
	*add_current = ((flags & (GX_ALLDIRS | GX_ADDCURDIR))
				== (GX_ALLDIRS | GX_ADDCURDIR));
	while (1)
	{
		if (interrupt_state || terminating_signal)
		{
			*lose = 1;
			break ;
		}
		else if (signal_is_pending(SIGINT))
		{
			*lose = 1;
			break ;
		}

		dp = readdir(d);
		if (dp == NULL)
			break;
		if (!REAL_DIR_ENTRY(dp))
			continue ;
		if (skipname())
			continue ;
		/* directories handling for GX_MATCHDIRS / GX_ALLDIRS */
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
				subdir = NULL;
				continue ;
			}
		}

		if (flags & GX_ALLDIRS)
		{
			if (isdir == 0)
			{
				e = NULL;
				ndirs = 0;
				dirlist;

				dirlist = finddirs(pat, subdir, (flags & ~GX_ADDCURDIR), &e, &ndirs);
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
			/* Always add this entry (even if not directory) */
			{
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
					break ;
				}
				nextlink->next = *lastlink;
				*lastlink = nextlink;
				nextlink->name = nextname;
				memcpy(nextname, subdir, sdlen + 1);
				free(subdir);
				subdir = NULL;
				++(*count);
				continue ;
			}
		}
		else if (flags & GX_MATCHDIRS)
		{
			free(subdir);
			subdir = NULL;
		}
		convfn = fnx_fromfs(dp->d_name, D_NAMLEN(dp));
		if (strmatch(convpat, convfn, mflags) != STRMATCH_NO)
		{
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
					FREE(nextlink);
				}
				FREE(nextname);
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

/* The refactored main function.  Mirrors original logic and returns
   the same values (pointer to error return, NULL on allocation failures,
   or pointer to allocated vector). */
char **
glob_vector (char *pat, char *dir, int flags)
{
	t_list *nextlink = NULL;
	t_list *lastlink = NULL;
	t_list *e = NULL;
	t_list *dirlist = NULL;
	unsigned int count = 0;
	int lose = 0;
	int skip = 0;
	int ndirs = 0;
	int isdir = -1;
	int add_current = 0;
	size_t patlen = 0;
	size_t sdlen = 0;
	char **name_vector = NULL;
	int mflags = 0;
	int pflags = 0;
	int hasglob = 0;
	int nalloca = 0;
	t_list *firstmalloc = NULL;
	t_list *tmplink = NULL;
	char *convfn = NULL;
	char *convpat = NULL;
	char *nextname = NULL;
	char *npat = NULL;
	char *subdir = NULL;
	int	addcur;
	int	r;
	t_list	*cur;
	t_list	*next;
	t_list	*tmp;

	/* Empty pat special-case */
	if (pat == NULL || *pat == '\0')
	{
		if (handle_empty_pat(dir, &lastlink, &firstmalloc, &nalloca, &count) < 0)
			return (char **) &glob_error_return;
		skip = 1;
	}

	patlen = (pat && *pat) ? strlen(pat) : 0;

	/* Non-glob optimization */
	hasglob = 0;
	if (!skip && ((hasglob = /*glob_pattern_p*/0) == 0 || hasglob == 2))
	{
		int h = handle_literal_pat(pat, patlen, dir, &lastlink, &firstmalloc, &nalloca, &count);
		if (h < 0)
			return (char **) &glob_error_return;
		if (h == 0)
			skip = 1;
	}

	/* Directory scanning when needed */
	if (!skip)
	{
		addcur = 0;
		r = scan_directory_and_collect(pat, dir, flags, &lastlink,
						   &firstmalloc, &nalloca, &count, &lose, &addcur);
		if (r < 0)
			return (char **) &glob_error_return;
		add_current = addcur;
	}

	/* Handle add_current compat */
	if (add_current && !lose)
	{
		sdlen = strlen(dir);
		nextname = (char *) malloc(sdlen + 1);
		nextlink = (t_list *) malloc(sizeof(struct globval));
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

	/* On error free everything we allocated */
	if (lose)
	{
		tmplink = NULL;
		free_globvals_on_error(lastlink, firstmalloc);
		return (NULL);
	}

	/* Copy names from linked list into vector (reverse order) */
	for (tmplink = lastlink, (size_t) (sdlen = 0); sdlen < count; ++sdlen)
	{
		name_vector[sdlen] = tmplink->name;
		tmplink = tmplink->next;
	}
	name_vector[count] = NULL;
	if (firstmalloc)
	{
		cur = lastlink;
		while (cur)
		{
			next = cur->next;
			if (cur == firstmalloc)
			{
				while (cur)
				{
					tmp = cur->next;
					free(cur);
					cur = tmp;
				}
				break ;
			}
			cur = next;
		}
	}

	return (name_vector);
}
