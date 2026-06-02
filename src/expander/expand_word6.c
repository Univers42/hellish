/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word6.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

static char	*build_proc_sub_result(t_shell *state, t_ast_node *cmd_word,
				bool is_input)
{
	t_string	cmd_str;
	char		*result;

	cmd_str = word_to_string(*cmd_word);
	if (!cmd_str.ctx)
		return (ft_strdup(BLACK_HOLE));
	if (!vec_ensure_space_n(&cmd_str, 1))
		return (free(cmd_str.ctx), NULL);
	((char *)cmd_str.ctx)[cmd_str.len] = '\0';
	if (is_input)
		result = create_procsub_input(state, (char *)cmd_str.ctx);
	else
		result = create_procsub_output(state, (char *)cmd_str.ctx);
	free(cmd_str.ctx);
	return (result);
}

char	*expand_proc_sub(t_shell *state, t_ast_node *node)
{
	t_token		*tok;
	t_ast_node	*cmd_word;
	bool		is_input;

	if (!node || node->node_type != AST_PROC_SUB || node->children.len < 2)
		return (NULL);
	tok = &((t_ast_node *)node->children.ctx)[0].token;
	cmd_word = &((t_ast_node *)node->children.ctx)[1];
	is_input = (tok->tt == TT_PROC_SUB_IN);
	return (build_proc_sub_result(state, cmd_word, is_input));
}
