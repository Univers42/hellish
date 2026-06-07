/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Push one path component onto the logical stack, applying POSIX -L rules
   textually: "." and empty components vanish, ".." pops the previous one
   (or is dropped at the root of an absolute path), anything else is kept.
   The stack stores borrowed pointers into the ft_split array. */
static void	cd_push_comp(t_vec *stack, char *comp)
{
	if (comp[0] == '\0' || !ft_strcmp(comp, "."))
		return ;
	if (!ft_strcmp(comp, ".."))
	{
		if (stack->len > 0)
			stack->len--;
		return ;
	}
	vec_push(stack, &comp);
}

/* Rejoin the kept components into an absolute path ("/a/b"), or "/" when the
   stack is empty. Returns a fresh string on the active heap. */
static char	*cd_build_path(t_vec *stack)
{
	t_string	s;
	size_t		i;

	vec_init(&s);
	s.elem_size = 1;
	if (stack->len == 0)
		vec_push_str(&s, "/");
	i = 0;
	while (i < stack->len)
	{
		vec_push_str(&s, "/");
		vec_push_str(&s, ((char **)stack->ctx)[i]);
		i++;
	}
	vec_push(&s, &(char){0});
	return ((char *)s.ctx);
}

/* Canonicalize an absolute path textually (no filesystem access): collapse
   "//", drop "." components and resolve ".." against the preceding component.
   This is the heart of logical (-L) cd: `/a/link/..` becomes `/a` without
   ever asking the kernel where the symlink really points. */
char	*cd_canonicalize(const char *path)
{
	char	**parts;
	t_vec	stack;
	int		i;
	char	*res;

	parts = ft_split(path, '/');
	if (!parts)
		return (ft_strdup("/"));
	vec_init(&stack);
	stack.elem_size = sizeof(char *);
	i = -1;
	while (parts[++i])
		cd_push_comp(&stack, parts[i]);
	res = cd_build_path(&stack);
	vec_destroy(&stack, NULL);
	free_tab(parts);
	return (res);
}

/* Build the destination of a logical cd: an absolute operand is canonicalized
   as-is; a relative one is first anchored to $PWD (the shell's idea of where
   it is, symlinks and all), then canonicalized. */
char	*cd_logical_path(t_shell *state, const char *target)
{
	char	*base;
	char	*pre;
	char	*joined;
	char	*res;

	if (target[0] == '/')
		return (cd_canonicalize(target));
	base = env_expand(state, PWD_NAME);
	if (!base || !base[0])
		base = (char *)state->cwd.ctx;
	if (!base)
		base = "/";
	pre = ft_strjoin(base, "/");
	if (!pre)
		return (NULL);
	joined = ft_strjoin(pre, target);
	xfree(pre);
	if (!joined)
		return (NULL);
	res = cd_canonicalize(joined);
	return (xfree(joined), res);
}
