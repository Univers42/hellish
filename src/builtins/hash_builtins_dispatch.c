/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins_repart.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:30 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:30 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

static void	init_builtin_hash(t_hash *h)
{
	hash_init(h, 32);
	hash_set(h, "echo", (void *)builtin_echo);
	hash_set(h, "export", (void *)builtin_export);
	hash_set(h, "cd", (void *)builtin_cd);
	hash_set(h, "exit", (void *)builtin_exit);
	hash_set(h, "pwd", (void *)builtin_pwd);
	hash_set(h, "env", (void *)builtin_env);
	hash_set(h, "unset", (void *)builtin_unset);
	hash_set(h, "type", (void *)builtin_type);
	hash_set(h, "set", (void *)builtin_set);
	hash_set(h, "read", (void *)builtin_read);
	hash_set(h, "test", (void *)builtin_test);
	hash_set(h, "[", (void *)builtin_test);
	hash_set(h, "alias", (void *)builtin_alias);
	hash_set(h, "unalias", (void *)builtin_unalias);
	hash_set(h, "hash", (void *)builtin_hash);
	hash_set(h, "jobs", (void *)builtin_jobs);
	hash_set(h, "fg", (void *)builtin_fg);
	hash_set(h, "bg", (void *)builtin_bg);
	hash_set(h, "fc", (void *)builtin_fc);
}

int	(*builtin_func(char *name))(t_shell *state, t_vec argv)
{
	static t_hash	h = {0};

	if (!h.ctx)
		init_builtin_hash(&h);
	return ((t_builtin_fn)hash_get(&h, name));
}
