/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   split_envvar.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 11:28:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 10:20:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Finalise curr_node as a complete field: push it to `ret` only if it has
   accumulated at least one token (empty fields between leading/trailing IFS
   whitespace are discarded this way), then reinitialise curr_node as a
   fresh empty AST_WORD ready to collect the next field. */
void	push_and_reinit_curr_node(t_vec_nd *ret, t_ast_node *curr_node)
{
	if (curr_node->children.len)
		vec_push(ret, curr_node);
	*curr_node = (t_ast_node){.node_type = AST_WORD};
	vec_init(&curr_node->children);
	curr_node->children.elem_size = sizeof(t_ast_node);
}

/* Wrap a freshly allocated string in a TT_ENVVAR node and append it to
   curr_node.  This is how IFS-split pieces and $@ elements are represented
   inside the field accumulator before it is handed to the glob stage. */
void	push_new_env_child(t_ast_node *curr_node, char *new_start)
{
	t_ast_node	tmp;

	tmp = new_env_node(new_start);
	vec_push(&curr_node->children, &tmp);
}

/* True when `c` is a non-NUL IFS character.  The NUL guard is important:
   ft_strchr will find '\0' at the end of any string, so without it every
   end-of-string test would falsely report an IFS character. */
bool	is_ifs_char(char c, const char *ifs)
{
	return (c != '\0' && ifs != NULL && ft_strchr(ifs, c) != NULL);
}

/* Expand a quoted "$@": one field per positional parameter (NOT IFS-split, so
   params containing spaces stay intact). First/last fields join adjacent
   literal text, mirroring POSIX behaviour for pre"$@"post. */
void	emit_positional_at(t_shell *state, t_ast_node *curr_node, t_vec_nd *ret)
{
	char	*cnt;
	char	*k;
	char	*v;
	int		count;
	int		i;

	cnt = env_expand(state, "#");
	if (cnt)
		count = ft_atoi(cnt);
	else
		count = 0;
	i = 0;
	while (++i <= count)
	{
		k = ft_itoa(i);
		v = env_expand(state, k);
		xfree(k);
		if (i > 1)
			push_and_reinit_curr_node(ret, curr_node);
		if (v)
			push_new_env_child(curr_node, ft_strdup(v));
		else
			push_new_env_child(curr_node, ft_strdup(""));
	}
}
