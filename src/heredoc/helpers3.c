/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"

/* Length of a ${...} body in `line` starting at the '{' (index 0): scan to the
   matching '}', honouring nested braces, like the lexer's advance_brace_param.
   Returns the offset of the closing '}', or 0 if it is unterminated. */
static int	brace_span(const char *line)
{
	int	i;
	int	depth;

	i = 1;
	depth = 1;
	while (line[i] && depth > 0)
	{
		depth += (line[i] == '{');
		depth -= (line[i] == '}');
		if (depth == 0)
			return (i);
		i++;
	}
	return (0);
}

/* Push the expansion of a ${body} (operator/length/trim or plain name). */
static void	push_braced_value(t_shell *state, t_string *ff,
				const char *body, int blen)
{
	char	*fmt;
	char	*env;

	fmt = expand_param_format(state, body, blen, true);
	if (fmt)
		return (vec_push_str(ff, fmt), xfree(fmt));
	env = env_expand_n(state, (char *)body, blen);
	if (env)
		vec_push_str(ff, env);
}

/* Handle a ${...} braced parameter expansion at line[*i] == '{' (just past the
   '$'). On an unterminated brace, emit a literal '$' so the bytes are kept.
   Advances *i past the closing '}'. */
void	expand_braced(t_shell *state, int *i, t_string *full_file, char *line)
{
	int	close;

	if (!full_file->ctx)
		vec_init(full_file);
	close = brace_span(line + *i);
	if (close == 0)
	{
		vec_push_str(full_file, "$");
		return ;
	}
	push_braced_value(state, full_file, line + *i + 1, close - 1);
	*i += close + 1;
}
