/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   process_word_token.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 12:54:03 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/24 20:10:54 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

/* Attempt to expand a $((...)) arithmetic substitution at the current scan
   position.  If the three-char prefix IS_DOLLAR LPAREN LPAREN is found, a
   fresh t_expand_ctx is initialised from ctx->s+ctx->pos and
   process_arith_sub is called.  Any text preceding the substitution that
   has not yet been written to outbuf is flushed first so the output stays
   in order.  Returns true and advances ctx->pos on success. */
bool	try_arith_sub_ctx(t_word_token_ctx *ctx)
{
	const char		*s;
	int				consumed;
	bool			result;
	t_expand_ctx	ectx;

	s = ctx->tok->start + ctx->pos;
	result = (ctx->pos + 2 < ctx->total_len && s[0] == IS_DOLLAR
			&& s[1] == LPAREN && s[2] == LPAREN);
	if (result)
	{
		consumed = 0;
		ectx = init_expand(s, ctx->total_len - ctx->pos,
				ctx->outbuf, &consumed);
		if (ctx->pos > 0 && ctx->outbuf->len == 0)
			vec_push_nstr(ctx->outbuf, ctx->tok->start, (size_t)ctx->pos);
		if (process_arith_sub(ctx->state, &ectx))
		{
			ctx->pos += consumed;
			ctx->changed = true;
			return (true);
		}
	}
	return (false);
}

/* Attempt to expand $(...) at the current scan position.  Mirrors
   try_arith_sub_ctx but calls process_cmd_sub instead.  The arith check
   must run first (try_arith_sub_ctx) because $((…)) shares the $( prefix
   and would be mis-parsed as a command substitution otherwise. */
bool	try_cmd_sub_ctx(t_word_token_ctx *ctx)
{
	const char		*s;
	t_expand_ctx	ectx;
	int				consumed;

	s = ctx->tok->start + ctx->pos;
	if (ctx->pos + 1 < ctx->total_len && s[0] == IS_DOLLAR && s[1] == LPAREN)
	{
		consumed = 0;
		ectx = init_expand(s, ctx->total_len - ctx->pos,
				ctx->outbuf, &consumed);
		if (ctx->pos > 0 && ctx->outbuf->len == 0)
			vec_push_nstr(ctx->outbuf, ctx->tok->start, (size_t)ctx->pos);
		if (process_cmd_sub(ctx->state, &ectx))
		{
			ctx->pos += consumed;
			ctx->changed = true;
			return (true);
		}
	}
	return (false);
}

/* Inside `..`, backslash keeps its literal meaning except before `, $ or \. */
static char	*unescape_backtick(const char *s, int len)
{
	char	*out;
	int		i;
	int		j;

	out = xmalloc(len + 1);
	if (!out)
		return (NULL);
	i = 0;
	j = 0;
	while (i < len)
	{
		if (s[i] == '\\' && i + 1 < len
			&& (s[i + 1] == '`' || s[i + 1] == '$' || s[i + 1] == '\\'))
			i++;
		out[j++] = s[i++];
	}
	out[j] = '\0';
	return (out);
}

/* Handle a `...` backtick command substitution at the current scan position.
   Backticks have different escape semantics than $(...): backslash only
   escapes `, $, and \ inside them.  unescape_backtick strips those escapes
   before handing the text to capture_subshell_output so the subshell sees
   the command the user intended.  On an unmatched backtick, return false
   and leave the literal ` in place rather than treating it as an error. */
bool	try_backtick_ctx(t_word_token_ctx *ctx)
{
	const char	*s;
	char		*inner;
	char		*out;
	int			j;

	s = ctx->tok->start + ctx->pos;
	if (s[0] != '`')
		return (false);
	j = 1;
	while (ctx->pos + j < ctx->total_len && s[j] != '`')
		j += 1 + (s[j] == '\\' && s[j + 1] != '\0');
	if (ctx->pos + j >= ctx->total_len)
		return (false);
	if (ctx->pos > 0 && ctx->outbuf->len == 0)
		vec_push_nstr(ctx->outbuf, ctx->tok->start, (size_t)ctx->pos);
	inner = unescape_backtick(s + 1, j - 1);
	out = capture_subshell_output(ctx->state, inner);
	xfree(inner);
	if (out && *out)
		vec_push_nstr(ctx->outbuf, out, ft_strlen(out));
	xfree(out);
	ctx->pos += j + 1;
	ctx->changed = true;
	return (true);
}

void	push_single_char_ctx(t_word_token_ctx *ctx)
{
	const char	*s;
	char		c;

	s = ctx->tok->start + ctx->pos;
	c = s[0];
	vec_push(ctx->outbuf, &c);
	ctx->pos++;
}
