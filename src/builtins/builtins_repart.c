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

#include "builtins.h"

typedef int (*builtin_fn_t)(t_shell *, t_vec);

static void	init_builtin_hash(t_hash *h)
{
	hash_init(h, 16);
	hash_set(h, "echo", (void *)builtin_echo);
	hash_set(h, "export", (void *)builtin_export);
	hash_set(h, "cd", (void *)builtin_cd);
	hash_set(h, "exit", (void *)builtin_exit);
	hash_set(h, "pwd", (void *)builtin_pwd);
	hash_set(h, "env", (void *)builtin_env);
	hash_set(h, "unset", (void *)builtin_unset);
	//hash_set(h, "histoy", (void *)builtin_history);
}

int	(*builtin_func(char *name))(t_shell *state, t_vec argv)
{
	static t_hash h = {0};

	if (!h.ctx)
		init_builtin_hash(&h);
	return (builtin_fn_t)hash_get(&h, name);
}
