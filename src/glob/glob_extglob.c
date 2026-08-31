/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_extglob.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "case_match.h"

/* extglob in a FILENAME pattern -- `ls @(*.c|*.h)`.
**
** A path segment holding an extglob group is taken whole, as one token,
** and matched by case_match: an alternative is an arbitrary pattern, so
** splitting it into the walker's per-token stream would mean teaching that
** stream about alternation and backtracking, i.e. a second matcher.  There
** is already exactly one pattern matcher in this shell and that is the
** point -- `case x in @(a|b))` and `echo @(a|b)` cannot be allowed to
** disagree about what the pattern means.
**
** Only the SEGMENT is taken, never across a `/`: the directory walk still
** happens segment by segment, so `@(src|lib)/ *.c` descends normally.
*/

/* End of the path segment starting at `i`: the next `/` at paren depth 0,
   or the end of the pattern. Parens are counted so a `/` written inside a
   group belongs to the group rather than splitting the path. */
static int	seg_end(const char *p, int i, int len)
{
	int	depth;

	depth = 0;
	while (i < len)
	{
		if (p[i] == '(')
			depth++;
		else if (p[i] == ')' && depth > 0)
			depth--;
		else if (p[i] == '/' && depth == 0)
			return (i);
		i++;
	}
	return (len);
}

/* Does the segment starting at `i` contain a group at depth 0? Both
   spellings count: bash's `@(a|b)` and zsh's bare `(a|b)`.
     The zsh form is asked with xg_alt_group rather than the lexer's
   zsh_alt_ahead, so a group filling a WHOLE segment counts too -- `src/
   (glob|lexer)` is a directory pattern and there is no subshell here to
   confuse it with. That distinction is the lexer's alone; by the time a
   pattern reaches this file it is already one word. */
static bool	seg_has_group(const char *p, int i, int end)
{
	while (i < end)
	{
		if (extglob_ahead(p + i) || xg_alt_group(p + i))
			return (true);
		i++;
	}
	return (false);
}

/* At a segment boundary, claim the whole segment as one G_EXTGLOB token
   when it holds a group. Returns true when it did, so the tokenizer's
   per-character dispatch is skipped for those bytes. Never fires on a
   quoted word: quoting turns every metacharacter into a literal, and that
   has to include this one. */
bool	handle_extglob_token(t_tokenizer_ctx ctx)
{
	t_glob	g;
	int		end;

	if (ctx.quoted || (*ctx.i > 0 && ctx.pattern[*ctx.i - 1] != '/'))
		return (false);
	end = seg_end(ctx.pattern, *ctx.i, ctx.len);
	if (!seg_has_group(ctx.pattern, *ctx.i, end))
		return (false);
	g = init_glob(G_EXTGLOB, ctx.pattern + *ctx.i, end - *ctx.i);
	vec_push(ctx.ret, &g);
	*ctx.i = end;
	return (true);
}

/* Does the group at token `g` match the prefix name[0..cut)? */
static bool	xg_pfx(char *name, size_t cut, t_glob *g)
{
	char	*head;
	char	*pat;
	bool	ok;

	head = ft_strndup(name, cut);
	pat = ft_strndup((char *)g->start, (size_t)g->len);
	ok = case_match(head, pat);
	xfree(head);
	xfree(pat);
	return (ok);
}

/* Match one directory entry against a segment beginning with an extglob
   group.
     The group does not necessarily own the rest of the segment: the word
   reparser hands back `+(a)` and `?` as two tokens, so this walks every
   split point and lets the ordinary token matcher take the tail.  Claiming
   the whole name instead reported a match at the wrong pattern offset, and
   the walker then went looking for a SUBDIRECTORY to continue in.
     The leading-dot rule is enforced here rather than inside case_match: a
   hidden file is offered to a pattern only when the pattern asks for the
   dot literally, or dotglob is on.  case_match knows nothing about
   filenames and must not -- it is also the `case` matcher, where a leading
   dot is an ordinary character. */
size_t	match_g_extglob(char *name, t_vec_glob patt, size_t offset, bool first)
{
	t_glob	*g;
	size_t	cut;
	size_t	r;

	g = (t_glob *)patt.ctx + offset;
	if (name[0] == '.' && g->start[0] != '.' && !glob_dotglob())
		return (0);
	(void)first;
	if (finished_pattern(patt, offset))
	{
		if (xg_pfx(name, ft_strlen(name), g))
			return (offset + 1);
		return (0);
	}
	cut = -1;
	while (++cut <= ft_strlen(name))
	{
		r = 0;
		if (xg_pfx(name, cut, g))
			r = matches_pattern(name + cut, patt, offset + 1, false);
		if (r)
			return (r);
	}
	return (0);
}
