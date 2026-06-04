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

void	reclassify_keywords(t_deque_tok *tokens)
{
	size_t	i;
	t_token	*t;
	bool	cmd_pos;
	bool	after_redir;
	bool	for_name;

	cmd_pos = true;
	after_redir = false;
	for_name = false;
	i = 0;
	while (i < tokens->deqtok.len)
	{
		t = (t_token *)deque_idx(&tokens->deqtok, i++);
		if (step_token(t, &cmd_pos, &after_redir, &for_name))
			continue ;
		if (t->tt == TT_WORD && cmd_pos)
			reclassify_word(t);
		for_name = (t->tt == TT_FOR);
		cmd_pos = is_cmd_position(t->tt);
	}
}
