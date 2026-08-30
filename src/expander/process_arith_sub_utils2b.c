/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_arith_sub_utils2b.c                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 13:16:56 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 13:22:51 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

char	*arith_expand(t_shell *state, const char *expr, int len);

/* Complete a $((...)) expansion once the closing )) has been found at j.
   Steps: (1) extract the raw expression text between $((…)), (2) run
   process_word_token on it in case there are nested $(...) or `...` inside,
   (3) if any $ remains after that, expand variable names textually via
   expand_arith_vars, (4) evaluate the resulting numeric expression via
   arith_expand and push the result string to outbuf. */
bool	finish_arith_sub(t_shell *state, t_expand_ctx *ctx, int j)
{
	char	*result;
	char	*expr;
	t_token	tmp;

	tmp.start = ft_strndup(ctx->s + 3, (j - 2) - 3);
	tmp.len = (j - 2) - 3;
	tmp.tt = TT_WORD;
	tmp.allocated = true;
	process_word_token(state, &tmp);
	if (ft_strchr(tmp.start, '$'))
	{
		expr = expand_arith_vars(state, tmp.start, tmp.len);
		result = arith_expand(state, expr, (int)ft_strlen(expr));
		xfree(expr);
	}
	else
		result = arith_expand(state, tmp.start, tmp.len);
	xfree(tmp.start);
	if (result)
	{
		vec_push_nstr(ctx->outbuf, result, ft_strlen(result));
		xfree(result);
	}
	*ctx->consumed = j;
	return (true);
}

/* Closing )) drops depth by 2 (it closes the outer $(( )) brackets). */
void	handle_double_close_paren(int *depth, int *j)
{
	*depth -= 2;
	*j += 2;
}

/* A single ( that is NOT part of a (( bumps depth by 1 only. */
void	handle_single_open_paren(int *depth, int *j)
{
	(*depth)++;
	(*j)++;
}

/* A single ) that is NOT part of )) drops depth by 1. */
void	handle_single_close_paren(int *depth, int *j)
{
	(*depth)--;
	(*j)++;
}

/* The span of zsh's `$#name` at `j` -- the '#' plus the name -- or 0.
**
**     precmd_functions[$(($#precmd_functions+1))]=_z_precmd       (z.sh:247)
**
** The arithmetic pre-expander reads `#` as the one-character special `$#`
** and copies the name after it verbatim, which leaves `0precmd_functions`
** for the evaluator: an arithmetic error, and one that names a variable the
** author never wrote.  A bare `$#` has no name after it and yields 0 here,
** so the positional count is untouched in both dialects.
*/
int	zsh_len_span(t_shell *state, const char *s, int len, int j)
{
	int	k;

	if (!zsh_arrays(state) || j >= len || s[j] != '#')
		return (0);
	k = j + 1;
	if (k >= len || !(s[k] == '_' || ft_isalpha((unsigned char)s[k])))
		return (0);
	while (k < len && (s[k] == '_' || ft_isalnum((unsigned char)s[k])))
		k++;
	return (k - j);
}
