/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_make.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 18:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/16 18:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* make <TAB>: complete target names from the Makefile in the current
** directory, parsed in-process (no fork, make is never run).
**
** WHAT IS READ.  The first regular file of GNUmakefile, makefile, Makefile
** (GNU make's order, symlinks followed).  Anything else -- no file, a
** directory, an unreadable file -- is "not mine": NULL, so the word keeps
** filename completion.  At most 4 MiB are read.
**
** WHAT COUNTS.  `targets : ...` rule lines (`a b c:` gives three, `a::` is
** a rule).  Not offered: variable assignments, directives, recipe lines
** (TAB), define blocks, names with % or $, names starting with '.', and
** names outside [A-Za-z0-9_./+-].  Prerequisites and `## docs` are ignored.
** A backslash-newline is joined with NOTHING in between (GNU make puts a
** space), which keeps `lo\<nl>ng:` a single name.
**
** INCLUDES.  `include`, `-include` and `sinclude` with literal names are
** followed, relative to the cwd, ONE level deep: what an included file
** includes is not read (that also makes circular includes harmless).
**
** CACHE.  One entry in the shell (t_make_cache), valid while the chosen
** file's name, device, inode, mtime and size are unchanged; one stat per
** TAB.  LIMITATION: a change to an INCLUDED file alone does not invalidate
** it -- touch the Makefile.
**
** ORDER.  Targets used in history come first (frecency, see
** complete_make_rank.c), then the rest alphabetically.
**
** WHEN IT STAYS OUT.  A word starting with '-' or holding '=' or '/' is not
** a target; and if an earlier argument is -C, --directory*, -f, --file* or
** --makefile*, the Makefile is not the one in the cwd, so filenames win.
** Short options are only recognised alone (`-C dir`, `-fx`), not clustered
** (`-kC dir`).  Zero matching targets also gives NULL, so `make out.txt`
** still completes a file.
*/

#include "complete_make.h"
#include <readline/readline.h>

/* readline's generator protocol: state 0 starts over, then one match per
   call.  The iterator is the one documented function-local static, as in
   pc_generator (progcomp.c).  Strings go to readline via rl_dup. */
static char	*mk_generator(const char *text, int state)
{
	static size_t	i;
	t_vec			*v;
	char			*s;

	v = &(*zle_state_cell())->make_cache.order;
	if (!state)
		i = 0;
	while (i < v->len)
	{
		s = (char *)((const char **)v->ctx)[i++];
		if (!ft_strncmp(s, text, ft_strlen(text)))
			return (rl_dup(s));
	}
	return (NULL);
}

static int	mk_pre(const char *w, size_t n, const char *lit)
{
	return (n >= ft_strlen(lit) && !ft_strncmp(w, lit, ft_strlen(lit)));
}

/* Does an earlier argument point make somewhere else? */
static int	mk_diverts(t_pv_line *a)
{
	const char	*w;
	size_t		n;

	n = prov_word(a, &w);
	while (n)
	{
		if (n >= 2 && w[0] == '-' && (w[1] == 'C' || w[1] == 'f'))
			return (1);
		if (mk_pre(w, n, "--directory") || mk_pre(w, n, "--file")
			|| mk_pre(w, n, "--makefile"))
			return (1);
		n = prov_word(a, &w);
	}
	return (0);
}

/* The matches come out of mk_generator already in the order worth
   showing (mk_order), so readline must not sort them again: it does that
   AFTER this returns, so the flag is cleared here and cmd_completion sets
   it back at the start of every attempt (completion.c). */
char	**comp_make(t_shell *st, const char *text, t_pv_line *a)
{
	char	**m;

	if (text[0] == '-' || ft_strchr(text, '=') || ft_strchr(text, '/'))
		return (NULL);
	if (mk_diverts(a) || !mk_load(st))
		return (NULL);
	mk_order(st);
	m = rl_completion_matches(text, mk_generator);
	if (m)
		rl_sort_completion_matches = 0;
	return (m);
}
