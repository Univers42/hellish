/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_attr.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Variable attribute table for declare -i (integer) and declare -n
   (nameref). Kept tiny and scanned linearly; every accessor returns
   immediately when the table is empty, so the assignment and read hot
   paths pay nothing until a script actually uses declare -i/-n. */

/* The attribute kind of `name` ('i', 'n', or 0), or 0 when unattributed. */
char	attr_kind(t_shell *state, const char *name, int nlen)
{
	t_var_attr	*a;
	size_t		i;

	if (state->var_attrs.len == 0)
		return (0);
	i = 0;
	while (i < state->var_attrs.len)
	{
		a = &((t_var_attr *)state->var_attrs.ctx)[i++];
		if ((int)ft_strlen(a->name) == nlen
			&& ft_strncmp(a->name, name, nlen) == 0)
			return (a->kind);
	}
	return (0);
}

/* The nameref target of `name`, or NULL if not a nameref. */
char	*attr_target(t_shell *state, const char *name, int nlen)
{
	t_var_attr	*a;
	size_t		i;

	if (state->var_attrs.len == 0)
		return (NULL);
	i = 0;
	while (i < state->var_attrs.len)
	{
		a = &((t_var_attr *)state->var_attrs.ctx)[i++];
		if (a->kind == 'n' && (int)ft_strlen(a->name) == nlen
			&& ft_strncmp(a->name, name, nlen) == 0)
			return (a->target);
	}
	return (NULL);
}

/* Duplicate the optional nameref target: NULL stays NULL so plain -i
   entries carry no target allocation. */
static char	*dup_or_null(const char *target)
{
	if (target)
		return (ft_strdup(target));
	return (NULL);
}

/* Set (or replace) the attribute of `name`. */
void	attr_set(t_shell *state, const char *name, char kind,
			const char *target)
{
	t_var_attr	a;
	t_var_attr	*e;
	size_t		i;

	if (state->var_attrs.elem_size == 0)
	{
		vec_init(&state->var_attrs);
		state->var_attrs.elem_size = sizeof(t_var_attr);
	}
	i = 0;
	while (i < state->var_attrs.len)
	{
		e = &((t_var_attr *)state->var_attrs.ctx)[i++];
		if (ft_strcmp(e->name, name) == 0)
		{
			e->kind = kind;
			xfree(e->target);
			e->target = dup_or_null(target);
			return ;
		}
	}
	a.name = ft_strdup(name);
	a.kind = kind;
	a.target = dup_or_null(target);
	vec_push(&state->var_attrs, &a);
}

/* Free the whole table (session teardown). */
void	attr_clear(t_shell *state)
{
	t_var_attr	*a;
	size_t		i;

	i = 0;
	while (i < state->var_attrs.len)
	{
		a = &((t_var_attr *)state->var_attrs.ctx)[i++];
		xfree(a->name);
		xfree(a->target);
	}
	xfree(state->var_attrs.ctx);
	vec_init(&state->var_attrs);
	state->var_attrs.elem_size = sizeof(t_var_attr);
}
