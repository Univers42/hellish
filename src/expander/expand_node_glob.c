/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_node_glob.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:40:46 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:40:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static void	push_glob_no_split(t_vec *args, t_vec *glob_words)
{
	t_string	temp;
	size_t		j;

	vec_init(&temp);
	j = 0;
	while (j < glob_words->len)
	{
		vec_push_str(&temp, ((char **)glob_words->ctx)[j]);
		free(((char **)glob_words->ctx)[j]);
		if (j++ + 1 < glob_words->len)
			vec_push_char(&temp, ' ');
	}
	vec_push(args, &temp.ctx);
}

/* POSIX: when no_glob is set we stringify the word verbatim instead of
   scanning the filesystem. */
void	expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one,
			bool no_glob)
{
	t_vec		glob_words;
	t_string	temp;
	size_t		j;

	if (no_glob)
	{
		temp = word_to_string(*node);
		if (!temp.ctx)
			vec_push(args, &(char *){ft_strdup("")});
		else
			vec_push(args, &temp.ctx);
		return (free_ast(node));
	}
	glob_words = expand_word_glob(*node);
	if (get_g_sig()->should_unwind)
		return ;
	if (keep_as_one)
		return (push_glob_no_split(args, &glob_words),
			free(glob_words.ctx), free_ast(node));
	j = 0;
	while (j < glob_words.len)
		vec_push(args, &((char **)glob_words.ctx)[j++]);
	(free(glob_words.ctx), free_ast(node));
}
