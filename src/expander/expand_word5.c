/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_word5.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:41:16 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:41:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static char	*push_plain_literal(t_ast_node *src, t_vec *args)
{
	t_token	*t;
	char	*v;

	if (!word_is_plain_literal(src))
		return (NULL);
	t = &((t_ast_node *)src->children.ctx)[0].token;
	v = ft_strndup(t->start, t->len);
	vec_push(args, &v);
	return (v);
}

static char	*fast_path_expand(t_shell *state, t_ast_node *src,
				t_vec *args, bool keep_as_one)
{
	char	*v;

	v = push_plain_literal(src, args);
	if (v)
		return (v);
	v = try_pure_arith(state, src);
	if (v)
		return (vec_push(args, &v), v);
	v = try_simple_envvar(state, src);
	if (v)
	{
		v = ft_strdup(v);
		return (vec_push(args, &v), v);
	}
	if (keep_as_one)
	{
		v = try_simple_concat(state, src);
		if (v)
			return (vec_push(args, &v), v);
	}
	return (NULL);
}

/* Non-destructive variant: expand the words of `src` into `args`. */
void	expand_word_ro(t_shell *state, t_ast_node *src,
			t_vec *args, bool keep_as_one)
{
	t_ast_node	scratch;

	if (fast_path_expand(state, src, args, keep_as_one))
		return ;
	scratch = clone_ast(src);
	expand_word(state, &scratch, args, keep_as_one);
}

/* Assignment-value variant: like expand_word_ro but no pathname expansion. */
void	expand_word_assign_ro(t_shell *state, t_ast_node *src, t_vec *args)
{
	t_ast_node	scratch;

	if (fast_path_expand(state, src, args, true))
		return ;
	scratch = clone_ast(src);
	expand_word_glob_ctl(state, &scratch, args, EW_KEEP_AS_ONE | EW_NO_GLOB);
}
