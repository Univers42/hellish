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

/* Scan forward from s[2] (past the opening '$(') to find the matching ')'.
   Nested $(...) or plain (...) are tracked with `depth` so that
   $(echo $(uname)) terminates at the outer ')' rather than the inner one.
   Returns the index of the byte AFTER the closing ')', or -1 if unmatched
   (incomplete command substitution — just leave it unexpanded). */
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

/* Copy the command text from inside $(...) into a fresh NUL-terminated
   string so capture_subshell_output can treat it as a plain C string. */
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

/* Append the substitution output to outbuf, skipping an empty result
   entirely.  The subout string is always freed here regardless. */
static void	push_cmd_sub_result(t_string *outbuf, char *subout)
{
	if (!subout)
		subout = ft_strdup("");
	if (*subout)
		vec_push_nstr(outbuf, subout, ft_strlen(subout));
	xfree(subout);
}

/* Run the command substitution body: extract the inner string, run it,
   push the stripped output into ctx->outbuf, and report how many input
   bytes were consumed so the caller can advance its scan position. */
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

/* Recognise and process $(...) at the start of ctx->s.  On success the
   captured output is appended to outbuf, *consumed is set to the number of
   input bytes used (including the outer parens), and true is returned.
   Returns false with no side-effects if s does not start with $( or the
   substitution is malformed. */
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
