/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmdsub_inplace.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 12:00:00 by marvin            #+#    #+#             */
/*   Updated: 2026/07/22 12:00:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "executor.h"
#include "ft_builtins.h"

/* Can this first word turn into MORE than one command?  A shell function body
   is a whole list; `eval`, `.`/`source` and `command` run arbitrary text; and
   a word containing '$' is an expansion we cannot resolve at scan time.  In
   every one of those the FIRST external command would execve in place and
   silently drop everything after it -- `f() { a; b; }; $(f)` printed only a's
   output.  A leading NAME=VALUE is rejected too rather than scanned past: it
   is rare inside $( ) and forking for it costs one clone, not correctness.
   So only a literal external command name is ever eligible. */
static bool	cs_word_opaque(t_shell *state, const char *s, int len)
{
	char	*w;
	bool	bad;

	if (len <= 0)
		return (true);
	w = ft_strndup(s, (size_t)len);
	if (!w)
		return (true);
	bad = (ft_strchr(w, '$') != NULL || ft_strchr(w, '=') != NULL
			|| func_lookup(state, w) != NULL || builtin_func(w) != NULL);
	xfree(w);
	return (bad);
}

/* Is the body ONE simple external command?  If so this disposable $( ) child
   has nothing left to do after it and execute_cmd_bg can execve in place
   instead of forking a grandchild -- `$(/bin/true)` then costs one clone like
   bash and dash, not two.  exec is irreversible, so the scan errs hard toward
   forking: every control operator, backquote and newline rejects, because any
   of them can mean "another command follows" (`$(a; b)`, `$(a && b)`,
   `$(a | b)`, `$(a &)`).  Plain < and > deliberately do NOT reject -- a
   redirect never introduces a second command, and run_cmd_in_place applies it
   with the very same set_up_redirection the forked child used; that is what
   lets us match dash on `$(cmd >file)`, where bash still forks twice.  The
   dangerous redirect spellings still reject on their other byte: `>&` and
   `>|` on the & / |, `<(` on the paren, a heredoc on its newline.  Quoted
   spans and nested $( ) / ${ } are skipped wholesale, and $(( )) rejects.
   Aliases reject outright: exec_string splices them AFTER this scan, so
   `alias f='a; b'` could otherwise smuggle a second command past us. */

/* The byte walk itself, from position i: false the moment any byte could
   start a second command; quoted spans and nested $( ) / ${ } are skipped
   wholesale (a skip helper answers -1 on an unterminated span, which the
   i >= 0 guard turns into a reject). */
static bool	cs_scan_rest(const char *s, int i)
{
	while (i >= 0 && s[i])
	{
		if (s[i] == '\'' || s[i] == '"')
			i = csf_skip_quoted(s, i);
		else if (s[i] == '$' && s[i + 1] == '(' && s[i + 2] == '(')
			return (false);
		else if (s[i] == '$' && s[i + 1] == '(')
			i = csf_skip_csub(s, i);
		else if (s[i] == '$' && s[i + 1] == '{')
			i = csf_skip_param(s, i);
		else if (ft_strchr("|&;`()\n", s[i]))
			return (false);
		else
			i++;
	}
	return (i >= 0);
}

bool	cs_single_cmd(t_shell *state, const char *s)
{
	int	i;
	int	w;

	if (!alias_table_empty(&state->aliases))
		return (false);
	i = 0;
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	w = i;
	while (s[w] && !ft_strchr(" \t", s[w]))
		w++;
	if (cs_word_opaque(state, s + i, w - i))
		return (false);
	return (cs_scan_rest(s, i));
}
