/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_arith_sub_utils.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:16:56 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 13:22:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "decomposer.h"

/* Parameter-expand $var/${var} in the arith text so they are substituted
   textually before evaluation (POSIX). The arith lexer only handled a plain
   $var in operand position, choking on ${...} or a var adjacent to base#num
   (10#$c, ${b}#ff). Bare names (no $) are left for the lexer's own lookup. */
static char	*expand_arith_vars(t_shell *state, t_token *tmp)
{
	t_ast_node	w;
	t_string	s;
	char		*ret;

	w = reparse_word((t_token){.start = tmp->start, .len = tmp->len,
			.tt = TT_WORD});
	expand_env_vars(state, &w, false);
	s = word_to_string(w);
	ret = ft_strndup(s.ctx ? (char *)s.ctx : "", s.len);
	(free(s.ctx), free_ast(&w));
	return (ret);
}

bool	finish_arith_sub(t_shell *state, t_expand_ctx *ctx, int j)
{
	char	*result;
	char	*expr;
	t_token	tmp;

	tmp = (t_token){.start = ft_strndup(ctx->s + 3, (j - 2) - 3),
		.len = (j - 2) - 3, .tt = TT_WORD, .allocated = true};
	process_word_token(state, &tmp);
	if (ft_strchr(tmp.start, '$'))
	{
		expr = expand_arith_vars(state, &tmp);
		result = arith_expand(state, expr, (int)ft_strlen(expr));
		free(expr);
	}
	else
		result = arith_expand(state, tmp.start, tmp.len);
	free(tmp.start);
	if (result)
		(vec_push_nstr(ctx->outbuf, result, ft_strlen(result)), free(result));
	*ctx->consumed = j;
	return (true);
}

void	handle_double_close_paren(int *depth, int *j)
{
	*depth -= 2;
	*j += 2;
}

void	handle_single_open_paren(int *depth, int *j)
{
	(*depth)++;
	(*j)++;
}

void	handle_single_close_paren(int *depth, int *j)
{
	(*depth)--;
	(*j)++;
}
