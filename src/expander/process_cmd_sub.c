/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

// Helper: find the closing parenthesis for command substitution
static int	find_cmd_sub_end(const char *s, int slen)
{
	int	depth;
	int	j;

	depth = 1;
	j = 2;
	while (j < slen && depth > 0)
	{
		if (is_single_open_paren(s, j))
			handle_single_open_paren(&depth, &j);
		else if (is_single_close_paren(s, j))
			handle_single_close_paren(&depth, &j);
		else
			j++;
	}
	if (depth != 0)
		return (-1);
	return (j);
}

// Helper: extract the command string inside $(...)
static char	*extract_cmd_inner(const char *s, int inlen)
{
	char	*inner;

	inner = xmalloc(inlen + 1);
	if (!inner)
		return (NULL);
	ft_memcpy(inner, s + 2, inlen);
	inner[inlen] = '\0';
	return (inner);
}

// Helper: push the result of command substitution into outbuf
static void	push_cmd_sub_result(t_string *outbuf, char *subout)
{
	if (!subout)
		subout = ft_strdup("");
	if (*subout)
		vec_push_nstr(outbuf, subout, ft_strlen(subout));
	xfree(subout);
}

// Helper: encapsulate the main logic for command substitution
static bool	do_cmd_sub(t_shell *state, t_expand_ctx *ctx, int j)
{
	int		inlen;
	char	*inner;
	char	*subout;

	inlen = (j - 1) - 2;
	inner = extract_cmd_inner(ctx->s, inlen);
	if (!inner)
		return (false);
	subout = capture_subshell_output(state, inner);
	xfree(inner);
	push_cmd_sub_result(ctx->outbuf, subout);
	*ctx->consumed = j;
	return (true);
}

/* Try to process command substitution $(...) at start of `s`. On success push
   captured output into outbuf, set *consumed and return true. */
bool	process_cmd_sub(t_shell *state, t_expand_ctx *ctx)
{
	int	j;

	if (!ctx || !ctx->s || !ctx->outbuf || !ctx->consumed)
		return (false);
	if (ctx->slen < 3 || ctx->s[0] != '$' || ctx->s[1] != '(')
		return (false);
	j = find_cmd_sub_end(ctx->s, ctx->slen);
	if (j == -1)
		return (false);
	return (do_cmd_sub(state, ctx, j));
}
