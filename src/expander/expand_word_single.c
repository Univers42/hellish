/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word_single.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:31:50 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:31:50 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

char *expand_word_single(t_shell *state, t_ast_node *curr)
{
	t_vec args;
	size_t i;
	char *temp;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_word(state, curr, &args, false);
	/* If expansion produced exactly one item, return it (transfer ownership) */
	if (args.len == 1)
	{
		temp = ((char **)args.ctx)[0];
		free(args.ctx);
		return (temp);
	}
	/* If expansion produced zero items, return the original word string
	   (behaviour: unmatched glob stays literal). */
	if (args.len == 0)
	{
		t_string s = word_to_string(*curr);
		/* ensure NUL-termination */
		if (!vec_ensure_space_n(&s, 1))
		{
			/* allocation failure: cleanup and return NULL to indicate error */
			free(s.ctx);
			return (NULL);
		}
		((char *)s.ctx)[s.len] = '\0';
		/* transfer ownership of s.ctx to caller */
		return ((char *)s.ctx);
	}
	/* args.len > 1: ambiguous expansion -> cleanup and return NULL */
	i = 0;
	while (i < args.len)
	{
		free(((char **)args.ctx)[i]);
		i++;
	}
	free(args.ctx);
	return (0);
}