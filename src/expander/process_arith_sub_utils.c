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
#include "env.h"

char	*expand_param_format(t_shell *state, const char *s, int slen, bool dq);

static void	push_braced_val(t_arith_expand_ctx *ctx, int start, int j)
{
	t_token	tok;

	tok = (t_token){.tt = TT_ENVVAR, .start = (char *)ctx->s + start,
		.len = j - start, .allocated = false};
	expand_token(ctx->state, &tok, false);
	if (tok.start)
		vec_push_nstr(ctx->out, tok.start, tok.len);
	if (tok.allocated)
		free_token_res(&tok);
}

/* Expand a ${name} or ${name:-word} reference inside arithmetic text.
   We re-use expand_param_format for the format operators (so ${#x} etc.
   work in $(( ${#var} + 1 ))), falling back to plain env_expand_n for a
   bare name.  The brace-depth scanner is hand-rolled here rather than
   re-using the full lexer to keep this path fast and dependency-free. */
static void	append_braced(t_arith_expand_ctx *ctx)
{
	int	start;
	int	depth;
	int	j;

	start = *ctx->i + 1;
	depth = 1;
	j = start;
	while (j < ctx->len && depth)
	{
		if (ctx->s[j] == '{')
			depth++;
		else if (ctx->s[j] == '}' && --depth == 0)
			break ;
		j++;
	}
	push_braced_val(ctx, start, j);
	if (j < ctx->len)
		*ctx->i = j + 1;
	else
		*ctx->i = j;
}

/* Append the value of a $name / $digit / $special parameter. */
static void	append_simple(t_arith_expand_ctx *ctx)
{
	int		j;
	char	*v;

	j = *ctx->i;
	if (ft_isdigit((unsigned char)ctx->s[j]))
		j++;
	else if (ctx->s[j] == '_' || ft_isalpha((unsigned char)ctx->s[j]))
		while (j < ctx->len
			&& (ctx->s[j] == '_' || ft_isalnum((unsigned char)ctx->s[j])))
			j++;
	else if (ctx->s[j] && ft_strchr("#?$!@*-", ctx->s[j]))
		j++;
	else
		return ((void)vec_push_char(ctx->out, '$'));
	v = env_expand_n(ctx->state, (char *)ctx->s + *ctx->i, j - *ctx->i);
	if (v)
		vec_push_str(ctx->out, v);
	*ctx->i = j;
}

/* Walk the arithmetic expression text and expand every $name / ${...}
   reference into its string value.  The result is a pure-numeric string
   that arith_expand can evaluate without seeing any $ characters.
   Everything that is not a $ is copied verbatim (operators, literals). */
static void	arith_vars_loop(t_arith_expand_ctx *ctx)
{
	const char	*s;
	int			len;

	s = ctx->s;
	len = ctx->len;
	while (*ctx->i < len)
	{
		if (s[*ctx->i] == '$' && *ctx->i + 1 < len && s[*ctx->i + 1] == '{')
		{
			(*ctx->i)++;
			append_braced(ctx);
		}
		else if (s[*ctx->i] == '$' && *ctx->i + 1 < len)
		{
			(*ctx->i)++;
			append_simple(ctx);
		}
		else
		{
			vec_push_char(ctx->out, s[*ctx->i]);
			(*ctx->i)++;
		}
	}
}

/* Public entry: expand all $var / ${...} references in arithmetic expression
   text `s[0..len)` and return the result as a freshly allocated string.
   Called by finish_arith_sub when the expression contains a '$' after
   process_word_token has already handled any nested $(...) / `...`. */
char	*expand_arith_vars(t_shell *state, const char *s, int len)
{
	t_string			out;
	t_arith_expand_ctx	ctx;
	int					i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	ctx.state = state;
	ctx.s = s;
	ctx.len = len;
	ctx.out = &out;
	ctx.i = &i;
	arith_vars_loop(&ctx);
	if (!out.ctx)
		return (ft_strdup(""));
	return ((char *)out.ctx);
}
