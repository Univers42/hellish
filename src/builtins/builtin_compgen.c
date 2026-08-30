/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compgen.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

/* `compgen` -- ask the shell what it would offer as completions, and print
** the answers one per line.
**
** It did not exist, which is what left `git-completion.bash` defining 140
** functions and doing nothing (#72 phase 4): every one of those functions
** ends in a `compgen` call, so the whole file loaded, registered, and
** produced an empty list for every word.
**
** The contract that matters is the exit STATUS, not the output: bash
** returns 1 when nothing matched, and completion functions branch on that.
** A version that always returned 0 would look right in a terminal and be
** wrong in every script that uses it.
*/

/* Print `s` when it starts with `pfx`; returns 1 if it did, so the caller
   can accumulate "did anything match at all". */
int	cg_emit(const char *s, const char *pfx)
{
	if (ft_strncmp((char *)s, (char *)pfx, ft_strlen((char *)pfx)) != 0)
		return (0);
	ft_printf("%s\n", s);
	return (1);
}

/* -W: the word list is split on IFS-ish whitespace and each field offered.
   bash also expands the list first; we take it as written, which is what
   every real caller passes (a literal list of flags or subcommands). */
static int	cg_words(const char *list, const char *pfx)
{
	char	**w;
	int		i;
	int		hit;

	w = ft_split((char *)list, ' ');
	if (!w)
		return (0);
	i = 0;
	hit = 0;
	while (w[i])
		hit += cg_emit(w[i++], pfx);
	free_tab(w);
	return (hit);
}

/* The action letter for -A NAME, so the two spellings share one path.
   Unknown names answer 0, which the caller reports as a usage error rather
   than silently completing nothing. */
char	cg_action_of(const char *name)
{
	if (ft_strcmp((char *)name, "function") == 0)
		return ('A');
	if (ft_strcmp((char *)name, "variable") == 0)
		return ('v');
	if (ft_strcmp((char *)name, "command") == 0)
		return ('c');
	if (ft_strcmp((char *)name, "builtin") == 0)
		return ('b');
	if (ft_strcmp((char *)name, "keyword") == 0)
		return ('k');
	if (ft_strcmp((char *)name, "file") == 0)
		return ('f');
	if (ft_strcmp((char *)name, "directory") == 0)
		return ('d');
	if (ft_strcmp((char *)name, "alias") == 0)
		return ('a');
	return (0);
}

/* Run every requested source against the prefix. Sources accumulate: bash
   allows `compgen -W list -f pfx` and prints both lists. */
static int	cg_run(t_shell *st, t_cgopt *o, const char *pfx)
{
	int	hit;

	hit = 0;
	if (o->words)
		hit += cg_words(o->words, pfx);
	if (o->act)
		hit += cg_source(st, o->act, pfx);
	return (hit);
}

/* compgen [-abcdfkv] [-A action] [-W wordlist] [word]
   Prints the matches, one per line; status 1 when there were none, which
   is how a completion function detects "nothing to offer".
     With NO source requested at all there is nothing to have failed to
   match, and bash answers 0 -- so "asked for nothing" and "asked for
   something and got nothing" stay distinguishable. */
int	builtin_compgen(t_shell *state, t_vec argv)
{
	t_cgopt		o;
	const char	*pfx;
	size_t		i;

	o = (t_cgopt){0};
	i = cg_parse_opts(state, argv, &o);
	if (i == CG_OPT_ERR)
		return (2);
	if (!o.words && !o.act)
		return (0);
	pfx = "";
	if (i < argv.len)
		pfx = ((char **)argv.ctx)[i];
	return (cg_run(state, &o, pfx) == 0);
}
