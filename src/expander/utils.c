/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:58 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Check if a command string is empty or whitespace-only */
bool	is_empty_command(const char *cmd)
{
	if (!cmd)
		return (true);
	while (*cmd)
	{
		if (*cmd != ' ' && *cmd != '\t' && *cmd != '\n')
			return (false);
		cmd++;
	}
	return (true);
}

bool	is_export(t_ast_node word)
{
	t_ast_node	c;

	if (word.children.len != 1)
		return (false);
	c = ((t_ast_node *)word.children.ctx)[0];
	if (c.token.tt != TT_WORD)
		return (false);
	if (ft_strncmp(c.token.start, "export", c.token.len))
		return (false);
	return (true);
}

int	expand_simple_cmd_redir(t_shell *state,
		t_expander_simple_cmd *exp, t_vec_int *redirects)
{
	int			redir_idx;

	if (redirect_from_ast_redir(state, exp->curr, &redir_idx))
	{
		get_g_sig()->should_unwind = 0;
		return (AMBIGUOUS_REDIRECT);
	}
	vec_push_int(redirects, redir_idx);
	return (0);
}

t_token_old	get_old_token(t_ast_node word)
{
	t_token_old	ret;

	if (word.node_type != AST_WORD || word.children.len == 0
		|| !word.children.ctx)
		return (init_token_old());
	ret = ((t_ast_node *)word.children.ctx)[0].token.full_word;
	return (ret);
}

bool	token_starts_with(t_token t, char *str)
{
	if (t.len < (int)ft_strlen(str))
		return (false);
	return (ft_strncmp(t.start, str, ft_strlen(str)) == 0);
}
