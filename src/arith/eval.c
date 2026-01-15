/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eval.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/15 15:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith.h"
#include "shell.h"
#include "libft.h"
#include <stdlib.h>

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
	/* Check that we consumed all input */
	if (!parser.error && lexer.current.type != ATOK_EOF)
		parser.error = true;
	if (parser.error || lexer.error)
	{
		*error = true;
		return (0);
	}
	return (result);
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
	char		buf[32];
	int			i;
	int			neg;
	//long long	tmp;

	result = arith_eval(state, expr, len, &error);
	if (error)
	{
		ft_eprintf("%s: %.*s: syntax error in arithmetic expression\n",
			state->context, len, expr);
		return (NULL);
	}
	/* Convert result to string */
	neg = 0;
	if (result < 0)
	{
		neg = 1;
		result = -result;
	}
	i = 31;
	buf[i] = '\0';
	if (result == 0)
		buf[--i] = '0';
	else
	{
		while (result > 0)
		{
			buf[--i] = '0' + (result % 10);
			result /= 10;
		}
	}
	if (neg)
		buf[--i] = '-';
	str = ft_strdup(buf + i);
	return (str);
}
