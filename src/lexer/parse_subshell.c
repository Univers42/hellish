/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 21:16:47 by marvin            #+#    #+#             */
/*   Updated: 2026/01/19 21:16:47 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "casescan.h"

char	*parse_quote(t_deque_tok *tokens, char **str, char q);

/* One step of the span scan: backslash escapes and quoted spans are
   consumed here (they are word material, so they close command position);
   everything else goes through the shared casescan automaton, which owns
   the paren depth and the case-pattern exception. Returns a continuation
   prompt when a quote is left open, NULL otherwise. */
static char	*subshell_step(t_deque_tok *tokens, char **str,
				t_casescan *cs, int *depth)
{
	int		i;
	char	*res;

	if (**str == '\\')
	{
		advance_bs(str);
		cs->cmdpos = false;
	}
	else if (**str == '\'' || **str == '"')
	{
		res = parse_quote(tokens, str, **str);
		if (res)
			return (res);
		cs->cmdpos = false;
	}
	else
	{
		i = 0;
		*depth += casescan_step(cs, *str, &i, *depth);
		*str += i;
	}
	return (0);
}

/* Scan over a $(...) command substitution at lex time without tokenising
   its interior. We honour nesting, quoted spans and case patterns (via
   the shared casescan automaton — issue #95: the unbalanced `)` closing
   a case pattern must not terminate the substitution). The span is later
   re-lexed by the expander. Returns NULL on success or a continuation
   prompt if the `)` was never found before end-of-input. */
char	*tokenize_subshell(t_deque_tok *tokens, char **str)
{
	int			depth;
	t_casescan	cs;
	char		*res;

	(*str) += 2;
	depth = 1;
	casescan_init(&cs, -1);
	while (**str && depth > 0)
	{
		res = subshell_step(tokens, str, &cs, &depth);
		if (res)
			return (res);
	}
	if (depth > 0)
		return (tokens->looking_for = ')', "subshell> ");
	return (0);
}
