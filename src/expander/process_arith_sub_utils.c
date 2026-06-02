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

char	*expand_param_format(t_shell *state, const char *s, int slen);

/* Append the value of a ${...} parameter (handles ${name}, ${x:-d}, ${x#p}...).
   *i points just past '$'; advances past the closing '}'. */
static void	append_braced(t_shell *st, const char *s, int len, int *i,
				t_string *out)
{
	int		start;
	int		depth;
	int		j;
	char	*v;

	start = *i + 1;
	depth = 1;
	j = start;
	while (j < len && depth)
	{
		if (s[j] == '{')
			depth++;
		else if (s[j] == '}' && --depth == 0)
			break ;
		j++;
	}
	v = expand_param_format(st, s + start, j - start);
	if (v)
		(vec_push_str(out, v), free(v));
	else if ((v = env_expand_n(st, (char *)s + start, j - start)))
		vec_push_str(out, v);
	*i = (j < len) ? j + 1 : j;
}

/* Append the value of a $name / $digit / $special parameter. *i points just
   past '$'; advances past the name. A lone '$' is kept literal. */
static void	append_simple(t_shell *st, const char *s, int len, int *i,
				t_string *out)
{
	int		j;
	char	*v;

	j = *i;
	if (ft_isdigit((unsigned char)s[j]))
		j++;
	else if (s[j] == '_' || ft_isalpha((unsigned char)s[j]))
		while (j < len && (s[j] == '_' || ft_isalnum((unsigned char)s[j])))
			j++;
	else if (s[j] && ft_strchr("#?$!@*-", s[j]))
		j++;
	else
		return ((void)vec_push_char(out, '$'));
	v = env_expand_n(st, (char *)s + *i, j - *i);
	if (v)
		vec_push_str(out, v);
	*i = j;
}

/* Substitute $var / ${...} parameters in the arith text textually before
   evaluation (POSIX), without building an AST (the AST path was ~2x bash on a
   $-in-arith loop). Bare names stay for the arith lexer's own lookup; nested
   $(...)/$((...)) were already resolved by process_word_token. */
static char	*expand_arith_vars(t_shell *state, const char *s, int len)
{
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (i < len)
	{
		if (s[i] == '$' && i + 1 < len && s[i + 1] == '{')
			(i++, append_braced(state, s, len, &i, &out));
		else if (s[i] == '$' && i + 1 < len)
			(i++, append_simple(state, s, len, &i, &out));
		else
			(vec_push_char(&out, s[i]), i++);
	}
	if (!out.ctx)
		return (ft_strdup(""));
	return ((char *)out.ctx);
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
		expr = expand_arith_vars(state, tmp.start, tmp.len);
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
