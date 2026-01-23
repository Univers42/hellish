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

void	expand_node_glob(t_ast_node *node, t_vec *args, bool keep_as_one)
{
	t_vec		glob_words;
	t_string	temp;
	size_t		j;

	glob_words = expand_word_glob(*node);
	if (get_g_sig()->should_unwind)
		return ;
	vec_init(&temp);
	j = 0;
	while (j < glob_words.len)
	{
		if (!keep_as_one)
			vec_push(args, &((char **)glob_words.ctx)[j]);
		else
			(vec_push_str(&temp, ((char **)glob_words.ctx)[j]),
			free(((char **)glob_words.ctx)[j]));
		if (j++ + 1 < glob_words.len && keep_as_one)
			vec_push_char(&temp, ' ');
	}
	if (keep_as_one)
		vec_push(args, &temp.ctx);
	(free(glob_words.ctx), free_ast(node));
}
