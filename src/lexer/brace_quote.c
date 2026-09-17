/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_quote.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "helpers.h"

/* The parameter at the head of a ${...} body: a name, a run of digits or
   one special character, then an optional [subscript]. Returns the index
   just past it, or -1 when the body does not open with a parameter. */
static int	brace_param_end(const char *s, int len)
{
	int	i;
	int	depth;

	i = 0;
	if (len > 0 && ft_isdigit(s[0]))
	{
		while (i < len && ft_isdigit(s[i]))
			i++;
	}
	else if (len > 0 && is_var_name_p1(s[0]))
	{
		while (i < len && is_var_name_p2(s[i]))
			i++;
	}
	else if (len > 0 && s[0] && ft_strchr("@*#?-$!", s[0]))
		i++;
	else
		return (-1);
	depth = 0;
	while (i < len && s[i] && (depth > 0 || s[i] == '['))
	{
		depth += (s[i] == '[') - (s[i] == ']');
		i++;
	}
	return (i);
}

/* Is a single quote a QUOTE inside this ${...}? `s` points just past the
** `${`, `in_dq` says the expansion sits inside a double-quoted word.
**
** Outside double quotes, always. Inside them it depends on the operator,
** and bash --posix draws the line at the PATTERN operators:
**
**     x=;  echo "${x:-'}"       '                  a word: ' is ordinary
**     x=ab; echo "${x%'b'}"     a                  a pattern: ' quotes
**     x=ab; echo "${x%'b}"      unexpected EOF     ... so this is open
**
** The lexer used to treat ' as ordinary for every operator, so the last
** line was accepted, and "${x//'}'/_}" ended at the quoted brace. The
** expander then handed the reparser a pattern holding a lone quote, which
** ran off its slice into ft_assert -- a segfault on user input. The lexer
** (advance_brace_param) and the reparser (handle_envvar_brace) both ask
** this one function, so they cannot disagree about where a word ends. */
bool	brace_sq_live(const char *s, int len, int in_dq)
{
	int	i;

	if (!in_dq)
		return (true);
	i = brace_param_end(s, len);
	if (i < 0 || i >= len || !s[i])
		return (false);
	return (ft_strchr("#%/^,", s[i]) != NULL);
}
