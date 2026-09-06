/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_dollar_sub.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Public entry: expand a $(...) or $((...)) at the start of `s` into `outbuf`,
   returning the number of input chars consumed (0 if not a substitution). */
int	expand_dollar_sub(t_shell *state, const char *s, int slen,
		t_string *outbuf)
{
	int				consumed;
	t_expand_ctx	ctx;

	consumed = 0;
	ctx = init_expand(s, slen, outbuf, &consumed);
	if (process_arith_sub(state, &ctx))
		return (consumed);
	if (process_cmd_sub(state, &ctx))
		return (consumed);
	return (0);
}

/* Public entry for the heredoc reader: expand a `...` at the start of `s`
   into outbuf and return the bytes consumed, 0 when the closing backquote
   is missing -- the caller then keeps the ` literal, as try_backtick_ctx
   does for words.  Same escape rule as words: inside backquotes only \`,
   \$ and \\ are active, every other backslash reaches the command. */
int	expand_backquote_sub(t_shell *state, const char *s, t_string *outbuf)
{
	int		j;
	char	*inner;
	char	*out;

	if (s[0] != '`')
		return (0);
	j = 1;
	while (s[j] && s[j] != '`')
		j += 1 + (s[j] == '\\' && s[j + 1] != '\0');
	if (s[j] != '`')
		return (0);
	inner = unescape_backtick(s + 1, j - 1);
	out = capture_subshell_output(state, inner);
	xfree(inner);
	if (out && *out)
		vec_push_nstr(outbuf, out, ft_strlen(out));
	xfree(out);
	return (j + 1);
}
