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

/* True when the token deque contains nothing meaningful: either fewer than two
   entries (the deque always keeps a sentinel), or exactly one token that is a
   lone newline (an empty line). Saves the parser from wasting a full parse
   attempt on blank input. */
bool	is_empty_token_list(t_deque_tok *tokens)
{
	if (tokens->deqtok.len < 2)
		return (true);
	if (tokens->deqtok.len == 2
		&& ((t_token *)deque_idx(&tokens->deqtok, 0))->tt == TT_NEWLINE)
		return (true);
	return (false);
}
