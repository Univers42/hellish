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

void	reclassify_word(t_ltoken *t, char *base);
bool	is_redirect(t_tt tt);
bool	is_cmd_position(t_tt tt);

/* Advance the context flags for one token and decide whether to skip
   reclassification. We skip when:
     - We just saw a redirect and this is its target filename (after_redir).
     - We just saw a redirect operator itself (set after_redir for next turn).
     - The previous token was `for` and this is the loop variable name
       (for_name) -- `for do` is valid; `do` must not become TT_DO here.
   Returns true if the token should be left as TT_WORD. */
static bool	step_token(t_ltoken *t, bool *cmd_pos,
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
/* `function name { ... }` -- the brace has to stay a command opener.
**
** `{` is only promoted to TT_LBRACE while cmd_pos holds, and after
** `function name` it does not: two plain WORDs in a row look like a command
** and its argument. So the brace stayed TT_WORD and the parser reported
** `syntax error near unexpected token '}'` even once the parser understood
** the keyword. Same latch TT_COPROC already uses for the same reason -- the
** token after the NAME is still a command position.
**
** Deliberately narrow: only the literal word `function` arms it, and only
** the following word is affected, so `echo function` and a command actually
** named `function` are untouched (both covered in the test). */
static bool	is_function_kw(t_ltoken *t, const char *base)
{
	return (t->tt == TT_WORD && t->len == 8
		&& ft_strncmp(base + t->off, "function", 8) == 0);
}

void	reclassify_keywords(t_deque_tok *tokens)
{
	size_t		i;
	t_ltoken	*t;
	bool		cmd_pos;
	bool		flags[3];

	cmd_pos = true;
	flags[0] = false;
	flags[1] = false;
	flags[2] = false;
	i = 0;
	while (i < tokens->deqtok.len)
	{
		t = (t_ltoken *)deque_idx(&tokens->deqtok, i++);
		if (step_token(t, &cmd_pos, &flags[0], &flags[1]))
			continue ;
		if (t->tt == TT_WORD && cmd_pos)
			reclassify_word(t, tokens->base);
		flags[1] = (t->tt == TT_FOR);
		cmd_pos = is_cmd_position(t->tt);
		if (flags[2] && t->tt == TT_WORD)
			cmd_pos = true;
		flags[2] = (t->tt == TT_COPROC) || is_function_kw(t, tokens->base);
	}
}
