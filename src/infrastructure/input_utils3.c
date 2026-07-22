/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   input_utils3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:31:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:53:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "input_private.h"
#include "helpers.h"

char	*expand_history(t_shell *state, const char *input);

/* True if `s` ends with an odd number of backslashes followed by a newline --
   the POSIX line-continuation marker. The parity walk is necessary because
   "\\\\\n" (two backslashes + newline) is not a continuation: the backslashes
   escape each other and the newline terminates the command. */
bool	ends_with_bs_nl(t_string s)
{
	size_t	i;
	bool	unterminated;

	if (s.len == 0)
		return (false);
	i = s.len;
	unterminated = false;
	if (((char *)s.ctx)[--i] != '\n')
		return (false);
	while (i > 0)
	{
		i--;
		if (((char *)s.ctx)[i] == '\\')
			unterminated = !unterminated;
		else
			break ;
	}
	return (unterminated);
}

/* Strip trailing \<newline> continuations by popping the last two bytes (the
   backslash and the newline) and reading another line with a ">" prompt. This
   repeats until no continuation remains, so "echo hello \<CR>\<CR>" works. */
void	extend_bs(t_shell *state)
{
	char	*prompt;

	while (ends_with_bs_nl(state->input))
	{
		vec_pop(&state->input);
		vec_pop(&state->input);
		prompt = ft_strdup("> ");
		if (readline_cmd(state, &prompt))
			return ;
	}
}

/* True when the token deque contains nothing meaningful: nothing but the
   END sentinel, or only TT_NEWLINE tokens before it. The multi-newline
   case matters under batched input delivery: a run of comment/blank lines
   arrives as one batch and lexes to [\n \n ... END] — executing that would
   both trip the parser and clobber $? (bash keeps $? across blank lines). */
bool	is_empty_token_list(t_deque_tok *tokens)
{
	size_t	i;

	if (tokens->deqtok.len < 2)
		return (true);
	i = 0;
	while (i + 1 < tokens->deqtok.len)
	{
		if (((t_ltoken *)deque_idx(&tokens->deqtok, i))->tt != TT_NEWLINE)
			return (false);
		i++;
	}
	return (true);
}
