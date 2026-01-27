/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eval.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"

/*
 * Evaluate an arithmetic expression and return the result.
 * Sets *error to true if there was a parsing error.
 */
long long	arith_eval(t_shell *state, const char *expr, int len, bool *error)
{
	t_arith_lexer	lexer;
	t_arith_parser	parser;
	long long		result;

	*error = false;
	arith_lexer_init(&lexer, expr, len);
	parser.lexer = &lexer;
	parser.shell = state;
	parser.error = false;
	parser.error_msg = NULL;
	result = arith_parse_expr(&parser);
	if (!parser.error && lexer.current.type != ATOK_EOF)
		parser.error = true;
	if (parser.error || lexer.error)
	{
		*error = true;
		return (0);
	}
	return (result);
}

/* helper: convert long long to malloc'ed string (keeps original logic) */
static char	*arith_lltoa(long long value)
{
	char	buf[32];
	int		i;
	int		neg;

	neg = 0;
	if (value < 0)
	{
		neg = 1;
		value = -value;
	}
	i = 31;
	buf[i] = '\0';
	if (value == 0)
		buf[--i] = '0';
	else
	{
		while (value > 0)
		{
			buf[--i] = '0' + (value % 10);
			value /= 10;
		}
	}
	if (neg)
		buf[--i] = '-';
	return (ft_strdup(buf + i));
}

/*
 * Expand an arithmetic expression and return the result as a string.
 * Returns NULL on error.
 */
char	*arith_expand(t_shell *state, const char *expr, int len)
{
	long long	result;
	bool		error;
	char		*str;

	result = arith_eval(state, expr, len, &error);
	if (error)
	{
		ft_eprintf("%s: %.*s: syntax error in arithmetic expression\n",
			state->ctx, len, expr);
		return (NULL);
	}
	str = arith_lltoa(result);
	return (str);
}
