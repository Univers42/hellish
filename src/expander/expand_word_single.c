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

static char	*handle_single_arg(t_vec *args)
{
	char	*temp;

	temp = ((char **)args->ctx)[0];
	xfree(args->ctx);
	return (temp);
}

static char	*build_string_from_node(t_ast_node *curr)
{
	t_string	s;

	s = word_to_string(*curr);
	if (!vec_ensure_space_n(&s, 1))
		return (xfree(s.ctx), NULL);
	((char *)s.ctx)[s.len] = '\0';
	return ((char *)s.ctx);
}

static char	*free_args_ambiguous(t_vec *args)
{
	size_t	i;

	i = 0;
	while (i < args->len)
		xfree(((char **)args->ctx)[i++]);
	return (xfree(args->ctx), NULL);
}

char	*expand_word_single(t_shell *state, t_ast_node *curr)
{
	t_vec	args;

	vec_init(&args);
	args.elem_size = sizeof(char *);
	expand_word(state, curr, &args, false);
	if (args.len == 1)
		return (handle_single_arg(&args));
	if (args.len == 0)
		return (build_string_from_node(curr));
	return (free_args_ambiguous(&args));
}

/* Non-destructive single-word expansion: leaves `curr` intact so a loop body
   redirect can be re-expanded next iteration. Expands on a private clone. */
char	*expand_word_single_ro(t_shell *state, t_ast_node *curr)
{
	t_ast_node	scratch;

	scratch = clone_ast(curr);
	return (expand_word_single(state, &scratch));
}
