/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

void	expand_word(t_shell *state, t_ast_node *node,
					t_vec *args, bool keep_as_one)
{
	t_vec_nd	words;
	size_t		i;

	(expand_tilde_word(state, node), expand_cmd_substitutions(state, node));
	(expand_env_vars(state, node), vec_init(&words));
	words.elem_size = sizeof(t_ast_node);
	if (!keep_as_one)
		words = split_words(state, node);
	else
	{
		vec_push(&words, node);
		*node = (t_ast_node){};
	}
	i = -1;
	while (++i < words.len)
	{
		expand_node_glob(&((t_ast_node *)words.ctx)[i], args, keep_as_one);
		if (get_g_sig()->should_unwind)
			while (i < words.len)
				free_ast(&((t_ast_node *)words.ctx)[i++]);
		if (get_g_sig()->should_unwind)
			break ;
	}
	(free(words.ctx), free_ast(node));
	return ;
}

char	*expand_proc_sub(t_shell *state, t_ast_node *node)
{
	t_token		*tok;
	t_ast_node	*cmd_word;
	t_string	cmd_str;
	char		*result;
	bool		is_input;

	if (!node || node->node_type != AST_PROC_SUB || node->children.len < 2)
		return (NULL);
	tok = &((t_ast_node *)node->children.ctx)[0].token;
	cmd_word = &((t_ast_node *)node->children.ctx)[1];
	is_input = (tok->tt == TT_PROC_SUB_IN);
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
