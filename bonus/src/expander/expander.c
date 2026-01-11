/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expander.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:31:37 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:31:37 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
# include "expander.h"
#include "glob.h"

char	*expand_word_single(t_shell *state, t_ast_node *curr)
{
	t_vec	args;
	size_t		i;
	char		*temp;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_word(state, curr, &args, false);
	if (args.len != 1)
	{
		i = 0;
		while (i < args.len)
		{
			free(((char **)args.ctx)[i]);
			i++;
		}
		free(args.ctx);
		return (0);
	}
	temp = ((char **)args.ctx)[0];
	free(args.ctx);
	return (temp);
}

t_token_old	get_old_token(t_ast_node word)
{
	t_token_old	ret;

	ft_assert(word.node_type == AST_WORD);
	ft_assert(word.children.len);
	ret = ((t_ast_node *)word.children.ctx)[0].token.full_word;
	ft_assert(ret.present);
	return (ret);
}

void	expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one)
{
	t_vec	glob_words;
	t_string	temp;
	size_t		j;

	glob_words = expand_word_glob(*node);
	if (g_should_unwind)
		return ;
	vec_init(&temp);
	j = 0;
	while (j < glob_words.len)
	{
		if (!keep_as_one)
			vec_push(args, &((char **)glob_words.ctx)[j]);
		else
		{
			vec_push_str(&temp, ((char **)glob_words.ctx)[j]);
			free(((char **)glob_words.ctx)[j]);
		}
		if (j++ + 1 < glob_words.len && keep_as_one) {
			char ch = ' ';
			vec_push(&temp, &ch);
		}
	}
	if (keep_as_one)
		vec_push(args, &temp.ctx);
	free(glob_words.ctx);
	free_ast(node);
}

void	expand_word(t_shell *state, t_ast_node *node,
	t_vec *args, bool keep_as_one)
{
	t_vec_nd	words;
	size_t		i;

	expand_tilde_word(state, node);
	expand_env_vars(state, node);
	vec_init(&words);
	words.elem_size = sizeof(t_ast_node);
	if (!keep_as_one)
		words = split_words(state, node);
	else
	{
		vec_push(&words, node);
		*node = (t_ast_node){};
	}
	i = 0;
	while (i < words.len)
	{
		expand_node_glob(&((t_ast_node *)words.ctx)[i], args, keep_as_one);
		if (g_should_unwind)
			while (i < words.len)
				free_ast(&((t_ast_node *)words.ctx)[i++]);
		if (g_should_unwind)
			break ;
		i++;
	}
	free(words.ctx);
	free_ast(node);
	return ;
}
