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

static void	push_and_reinit_curr_node(t_vec_nd *ret, t_ast_node *curr_node)
{
	if (curr_node->children.len)
		vec_push(ret, curr_node);
	*curr_node = (t_ast_node){.node_type = AST_WORD};
	vec_init(&curr_node->children);
	curr_node->children.elem_size = sizeof(t_ast_node);
}

/* helper: create an env node from new_start and
	push it into curr_node->children */
static void	push_new_env_child(t_ast_node *curr_node, char *new_start)
{
	t_ast_node	tmp;

	tmp = new_env_node(new_start);
	vec_push(&curr_node->children, &tmp);
}

static bool	is_ifs_char(char c, const char *ifs)
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
	count = ((cnt) ? ft_atoi(cnt) : 0);
	i = 0;
	while (++i <= count)
	{
		k = ft_itoa(i);
		v = env_expand(state, k);
		free(k);
		if (i > 1)
			push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, ft_strdup(v ? v : ""));
	}
}

/* Field splitting for a non-whitespace IFS (e.g. IFS=,): empty fields are
   preserved. Emits all fields, the first joining any accumulated text. */
static void	split_envvar_nonws(t_token *curr_t, char **things,
				t_ast_node *curr_node, t_vec_nd *ret)
{
	int	i;

	if (curr_t->allocated)
		free((char *)curr_t->start);
	curr_t->allocated = false;
	if (!things[0])
		return ((void)free(things));
	push_new_env_child(curr_node, things[0]);
	i = 1;
	while (things[i])
	{
		push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, things[i++]);
	}
	free(things);
}

/*
** POSIX field splitting: leading/trailing IFS in the expanded value acts as a
** delimiter against adjacent literal text (so [$x] with x="  a  " -> [, a, ]),
** while a genuinely empty value creates no delimiter ([$x] x="" -> []).
*/
void	split_envvar(t_shell *state, t_token *curr_t,
			t_ast_node *curr_node, t_vec_nd *ret)
{
	char	**things;
	char	*ifs;
	bool	lead;
	bool	trail;
	int		i;

	if (!curr_t->start)
		return ;
	ifs = env_get_ifs(&state->env);
	if (ifs_has_nonws(ifs))
		return (split_envvar_nonws(curr_t,
				ifs_split_posix(curr_t->start, ifs), curr_node, ret));
	lead = is_ifs_char(curr_t->start[0], ifs);
	trail = (curr_t->start[0] != '\0'
			&& is_ifs_char(curr_t->start[ft_strlen(curr_t->start) - 1], ifs));
	things = ft_split_str(curr_t->start, ifs);
	if (curr_t->allocated)
		free((char *)curr_t->start);
	curr_t->allocated = false;
	if (lead && curr_node->children.len)
		push_and_reinit_curr_node(ret, curr_node);
	if (!things[0])
		return (free(things));
	push_new_env_child(curr_node, things[0]);
	i = 1;
	while (things[i])
	{
		push_and_reinit_curr_node(ret, curr_node);
		push_new_env_child(curr_node, things[i++]);
	}
	if (trail)
		push_and_reinit_curr_node(ret, curr_node);
	free(things);
}
