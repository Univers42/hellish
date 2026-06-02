/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   eval.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 15:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 03:46:59 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "arith_private.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

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
	parser.no_side_effects = false;
	parser.error_msg = NULL;
	if (lexer.current.type == ATOK_EOF)
		return (0);
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

/* long long -> malloc'd string. Operates on the magnitude as unsigned so that
   LLONG_MIN (whose signed negation overflows) is formatted correctly. */
static char	*arith_lltoa(long long value)
{
	char				buf[32];
	int					i;
	int					neg;
	unsigned long long	u;

	neg = (value < 0);
	u = (unsigned long long)value;
	if (neg)
		u = -u;
	i = 31;
	buf[i] = '\0';
	if (u == 0)
		buf[--i] = '0';
	while (u > 0)
	{
		buf[--i] = '0' + (int)(u % 10);
		u /= 10;
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
		ft_eprintf("%s: %.*s: arithmetic error\n", state->ctx, len, expr);
		state->last_cmd_st_exe = (t_execution_state){.status = 127};
		if (state->metinp != INP_RL)
			exit_clean(state, 127);
		return (NULL);
	}
	str = arith_lltoa(result);
	return (str);
}
