/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   assignment_to_env.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:08:32 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:08:32 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sys.h"

/* duplicate LHS key from assignment node */
static char	*dup_key_from_node(t_ast_node *node)
{
	return (ft_strndup(
			((t_ast_node *)node->children.ctx)[0].token.start,
		((t_ast_node *)node->children.ctx)[0].token.len
	));
}

/* duplicate first expanded arg value 
(transfer ownership of inner ptr), or "" */
static char	*dup_value_from_args(t_vec *args)
{
	char	*val;
	char	*dup;

	if (!args->len)
		return (NULL);
	ft_assert(args->len == 1);
	val = ((char **)args->ctx)[0];
	if (val)
	{
		dup = ft_strdup(val);
		return (free(val), dup);
	}
	return (ft_strdup(""));
}

/* trim leading '=' characters, return newly
 allocated string (preserve non-null) */
static char	*trim_leading_equals(char *s)
{
	size_t	k;
	char	*trimmed;

	if (!s || s[0] != EQ)
		return (s);
	k = 0;
	while (s[k] == EQ)
		k++;
	trimmed = ft_strdup(s + k);
	free(s);
	if (trimmed)
		return (trimmed);
	return (ft_strdup(""));
}

t_env	assignment_to_env(t_shell *state, t_ast_node *node)
{
	t_vec	args;
	t_env	ret;
	char	*v;

	ret = (t_env){.exported = false};
	vec_init(&args);
	args.elem_size = sizeof(char *);
	ft_assert(node->children.len == 2);
	expand_word(state,
		&((t_ast_node *)node->children.ctx)[1], &args, true);
	ft_assert(((t_ast_node *)node->children.ctx)
	[1].children.ctx == 0);
	ret.key = dup_key_from_node(node);
	if (args.len)
	{
		v = dup_value_from_args(&args);
		if (v)
			ret.value = trim_leading_equals(v);
		else
			ret.value = ft_strdup("");
	}
	return (free(args.ctx), ret);
}
