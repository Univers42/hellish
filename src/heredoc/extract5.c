/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extract5.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The command line a `<<` sits on is a LOGICAL line. A heredoc body starts
   after the newline TOKEN that ends its command, and a newline inside a
   quote, `...`, $( ) or ${ } is word text, not that token: bash reads the
   body of `cat <<E; echo 'a<NL>b'` after the `b'` line. Lexing one
   physical line alone (the old way) read `b' | cat <<E` as an OPEN quote,
   so the operator was never seen, its body ran as commands, and the
   fallback that went looking for it read past the end of the script
   buffer (issue #139: nvm's `nvm install` quotes a multi-line awk program
   right before a `<<EOF`). So the line is grown one physical line at a
   time while the lexer still reports a lexeme open. */

#include "heredoc_private.h"
#include "lexer.h"

/* Start of the physical line after the one p is on. */
static const char	*hd_next_line(const char *p)
{
	while (*p && *p != '\n')
		p++;
	return (p + (*p == '\n'));
}

/* Join physical lines onto the logical line until one could close the
   lexeme the lexer is waiting on. A line without the awaited byte cannot,
   so it is joined unlexed: a 500-line quoted awk program costs one lex
   per line holding a quote, not one per line. */
static void	hd_join(const char **end, size_t *line, char want)
{
	const char	*ls;

	while (**end)
	{
		ls = *end;
		*end = hd_next_line(ls);
		(*line)++;
		if (!want || ft_memchr(ls, want, *end - ls))
			return ;
	}
}

/* Lex the logical line starting at ls into tt. *end becomes its end and
   *line the index of its LAST physical line -- where the bodies of its
   operators begin. At end of input an open lexeme is lexed as far as it
   goes, as before. Returns the copy tt's tokens point into (NULL when
   out of memory); the caller frees it once done with tt. */
char	*hd_lex_line(const char *ls, const char **end, size_t *line,
			t_deque_tok *tt)
{
	char	*copy;
	char	*open;

	*end = hd_next_line(ls);
	while (1)
	{
		copy = ft_strndup(ls, *end - ls);
		if (!copy)
			return (NULL);
		tt->looking_for = 0;
		open = tokenizer(copy, tt);
		if (!open || !**end)
			return (copy);
		xfree(copy);
		hd_join(end, line, tt->looking_for);
	}
}
