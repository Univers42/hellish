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

/* Try to process arithmetic $((...)) at start of `s`. On success push result
   into outbuf, set *consumed to number of bytes consumed and return true. */
static bool	process_arith_sub(t_shell *state, const char *s,
				int slen, int *consumed, t_string *outbuf)
{
	int		depth;
	int		j;
	char	*result;

	if (slen < 4 || s[0] != '$' || s[1] != '(' || s[2] != '(')
		return (false);
	depth = 2;
	j = 3;
	while (j < slen && depth > 0)
	{
		if (s[j] == '(' && j + 1 < slen && s[j + 1] == '(')
			depth += 2, j += 2;
		else if (s[j] == ')' && j + 1 < slen && s[j + 1] == ')')
			depth -= 2, j += 2;
		else if (s[j] == '(')
			depth++, j++;
		else if (s[j] == ')')
			depth--, j++;
		else
			j++;
	}
	if (depth != 0)
		return (false);
	/* expression is between s+3 .. s+(j-2)-1 inclusive */
	{
		int expr_len = (j - 2) - 3;
		result = arith_expand(state, (char *)s + 3, expr_len);
		if (result)
		{
			vec_push_nstr(outbuf, result, ft_strlen(result));
			free(result);
		}
		*consumed = j;
		return (true);
	}
}

/* Try to process command substitution $(...) at start of `s`. On success push
   captured output into outbuf, set *consumed and return true. */
static bool	process_cmd_sub(t_shell *state, const char *s,
				int slen, int *consumed, t_string *outbuf)
{
	int		depth;
	int		j;
	char	*inner;
	char	*subout;

	if (slen < 3 || s[0] != '$' || s[1] != '(')
		return (false);
	depth = 1;
	j = 2;
	while (j < slen && depth > 0)
	{
		if (s[j] == '(')
			depth++, j++;
		else if (s[j] == ')')
			depth--, j++;
		else
			j++;
	}
	if (depth != 0)
		return (false);
	{
		int inlen = (j - 1) - 2;
		inner = malloc(inlen + 1);
		if (!inner)
			return (false);
		memcpy(inner, s + 2, inlen);
		inner[inlen] = '\0';
		subout = capture_subshell_output(state, inner);
		free(inner);
		if (!subout)
			subout = ft_strdup("");
		if (*subout)
			vec_push_nstr(outbuf, subout, ft_strlen(subout));
		free(subout);
		*consumed = j;
		return (true);
	}
}

/* Process a TT_WORD token: perform arithmetic and command substitutions
   within its string and update token on change. */
void	process_word_token(t_shell *state, t_token *tok)
{
	t_string	outbuf;
	const char	*s;
	int			total_len;
	int			pos;
	bool		changed;

	vec_init(&outbuf);
	outbuf.elem_size = 1;
	total_len = tok->len;
	pos = 0;
	changed = false;
	while (pos < total_len)
	{
		s = tok->start + pos;
		/* remaining length */
		if (pos + 2 < total_len && s[0] == '$' && s[1] == '(' && s[2] == '(')
		{
			int consumed = 0;
			if (pos > 0 && outbuf.len == 0)
				vec_push_nstr(&outbuf, tok->start, (size_t)pos);
			if (process_arith_sub(state, s, total_len - pos, &consumed, &outbuf))
			{
				pos += consumed;
				changed = true;
				continue;
			}
		}
		if (pos + 1 < total_len && s[0] == '$' && s[1] == '(')
		{
			int consumed = 0;
			if (pos > 0 && outbuf.len == 0)
				vec_push_nstr(&outbuf, tok->start, (size_t)pos);
			if (process_cmd_sub(state, s, total_len - pos, &consumed, &outbuf))
			{
				pos += consumed;
				changed = true;
				continue;
			}
		}
		/* copy single char */
		{
			char c = s[0];
			vec_push(&outbuf, &c);
			pos++;
		}
	}
	if (changed)
	{
		char *newstr = malloc(outbuf.len + 1);
		if (newstr)
		{
			if (outbuf.len)
				memcpy(newstr, outbuf.ctx, outbuf.len);
			newstr[outbuf.len] = '\0';
			if (tok->allocated && tok->start)
				free((char *)tok->start);
			tok->start = newstr;
			tok->len = outbuf.len;
			tok->allocated = true;
		}
		free(outbuf.ctx);
	}
	else
		free(outbuf.ctx);
}
