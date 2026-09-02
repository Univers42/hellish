/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   keywords_zsh.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

bool	is_cmd_position(t_tt tt);

/* zsh's `} always {` needs a command position too, for the same reason the
   brace after `function name` does -- nothing about a bare WORD puts the
   scanner back at one, so the `{` stayed TT_WORD and the parser reported an
   error on it.  It arms cmd_pos DIRECTLY rather than through the flags[2]
   latch: that latch skips a token, which is right for `function NAME {` and
   one too many here, where the brace comes next.
   Only in the zsh dialect -- in bash `always` is an ordinary command name
   and a `{` after it is an ordinary argument. */
bool	is_always_kw(t_ltoken *t, const char *base, bool zsh)
{
	return (zsh && t->tt == TT_WORD && t->len == 6
		&& ft_strncmp(base + t->off, "always", 6) == 0);
}

/* Track brace nesting, and in the ZSH dialect promote a `}` that closes a
** group even though it is not at a command position:
**
**     gdv() { git diff -w "$@" | view - }
**
** is a function in zsh and a syntax error in bash, where `}` is a reserved
** word only at the start of a command -- here it is an argument to `view`.
** oh-my-zsh's git plugin has thirty of these, and it was the last thing
** stopping the file from loading.
**
** Gated twice over.  On the dialect, because `echo }` must go on printing a
** brace in bash mode; and on depth, because a `}` with no group open is an
** ordinary word in zsh too.  Nesting is counted from the promoted types, so
** a `{` that never became TT_LBRACE (a literal brace argument) does not open
** a group that a later `}` could close.
*/
void	brace_step(t_ltoken *t, const char *base, int *depth, bool zsh)
{
	if (zsh && *depth > 0 && t->tt == TT_WORD && t->len == 1
		&& base[t->off] == '}')
		t->tt = TT_RBRACE;
	else if (!zsh && *depth > 0 && t->tt == TT_WORD && t->len == 1
		&& base[t->off] == '}' && *zsh_brace_cell() < 0)
		*zsh_brace_cell() = (long)t->off;
	if (t->tt == TT_LBRACE)
		(*depth)++;
	else if (t->tt == TT_RBRACE && *depth > 0)
		(*depth)--;
}

/* Where the BASH dialect last saw a bare `}` word inside an open group, as
   an offset into the lexed text, or -1. Set by brace_step above and cleared
   by reclassify_keywords at the start of every pass, so it always describes
   the most recent lex of the cycle. The parser pops tokens as it consumes
   them, so by the time "unexpected end of file" is reported the deque is
   empty and only this cell still knows -- it is what lets the error say
   "line 3: a bare `}' closes a group only in zsh" (issue #112). */
long	*zsh_brace_cell(void)
{
	static long	off = -1;

	return (&off);
}
