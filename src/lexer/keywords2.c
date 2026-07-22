/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   keywords2.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

void	reclassify_word(t_token *t);
bool	is_redirect(t_tt tt);
bool	is_cmd_position(t_tt tt);

/* Advance the context flags for one token and decide whether to skip
   reclassification. We skip when:
     - We just saw a redirect and this is its target filename (after_redir).
     - We just saw a redirect operator itself (set after_redir for next turn).
     - The previous token was `for` and this is the loop variable name
       (for_name) -- `for do` is valid; `do` must not become TT_DO here.
   Returns true if the token should be left as TT_WORD. */
static bool	step_token(t_token *t, bool *cmd_pos,
						bool *after_redir, bool *for_name)
{
	if (*after_redir)
	{
		*after_redir = false;
		return (true);
	}
	if (is_redirect(t->tt))
	{
		*after_redir = true;
		return (true);
	}
	if (*for_name)
	{
		*for_name = false;
		*cmd_pos = true;
		return (true);
	}
	return (false);
}

/* Second pass over the token deque: upgrade TT_WORD tokens that appear in
   command position to their keyword types. This two-pass design lets us
   tokenise first (context-free) and classify keywords in a separate linear
   scan, keeping the tokenizer itself simple. The state machine tracks
   cmd_pos (are we at the start of a command?), after_redir (is the next
   word a redirect target?), and for_name (is the next word a for-variable?).
   `in` is handled separately by the parser, not here. */
void	reclassify_keywords(t_deque_tok *tokens)
{
	size_t	i;
	t_token	*t;
	bool	cmd_pos;
	bool	flags[3];

	cmd_pos = true;
	flags[0] = false;
	flags[1] = false;
	flags[2] = false;
	i = 0;
	while (i < tokens->deqtok.len)
	{
		t = (t_token *)deque_idx(&tokens->deqtok, i++);
		if (step_token(t, &cmd_pos, &flags[0], &flags[1]))
			continue ;
		if (t->tt == TT_WORD && cmd_pos)
			reclassify_word(t);
		flags[1] = (t->tt == TT_FOR);
		cmd_pos = is_cmd_position(t->tt);
		if (flags[2] && t->tt == TT_WORD)
			cmd_pos = true;
		flags[2] = (t->tt == TT_COPROC);
	}
}
