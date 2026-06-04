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

static void	fill_builtin_hash1(t_hash *h)
{
	hash_set(h, "echo", (void *)builtin_echo);
	hash_set(h, "export", (void *)builtin_export);
	hash_set(h, "cd", (void *)builtin_cd);
	hash_set(h, "pushd", (void *)builtin_pushd);
	hash_set(h, "popd", (void *)builtin_popd);
	hash_set(h, "exit", (void *)builtin_exit);
	hash_set(h, "pwd", (void *)builtin_pwd);
	hash_set(h, "env", (void *)builtin_env);
	hash_set(h, "unset", (void *)builtin_unset);
	hash_set(h, "type", (void *)builtin_type);
	hash_set(h, "set", (void *)builtin_set);
	hash_set(h, "shift", (void *)builtin_shift);
	hash_set(h, ":", (void *)builtin_colon);
	hash_set(h, "break", (void *)builtin_break);
	hash_set(h, "continue", (void *)builtin_continue);
	hash_set(h, "eval", (void *)builtin_eval);
	hash_set(h, ".", (void *)builtin_source);
	hash_set(h, "source", (void *)builtin_source);
	hash_set(h, "true", (void *)builtin_true);
	hash_set(h, "false", (void *)builtin_false);
	hash_set(h, "umask", (void *)builtin_umask);
	hash_set(h, "command", (void *)builtin_command);
}

static void	fill_builtin_hash2(t_hash *h)
{
	hash_set(h, "return", (void *)builtin_return);
	hash_set(h, "getopts", (void *)builtin_getopts);
	hash_set(h, "exec", (void *)builtin_exec);
	hash_set(h, "wait", (void *)builtin_wait);
	hash_set(h, "times", (void *)builtin_times);
	hash_set(h, "trap", (void *)builtin_trap);
	hash_set(h, "readonly", (void *)builtin_readonly);
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
	hash_set(h, "history", (void *)builtin_history);
	hash_set(h, "let", (void *)builtin_let);
	hash_set(h, "local", (void *)builtin_local);
	hash_set(h, "kill", (void *)builtin_kill);
	hash_set(h, "printf", (void *)builtin_printf);
	hash_set(h, "ulimit", (void *)builtin_ulimit);
	hash_set(h, "update", (void *)builtin_update);
}

static void	init_builtin_hash(t_hash *h)
{
	hash_init(h, 32);
	fill_builtin_hash1(h);
	fill_builtin_hash2(h);
}

int	(*builtin_func(char *name))(t_shell *state, t_vec argv)
{
	static t_hash	h = {0};

	if (!h.ctx)
		init_builtin_hash(&h);
	return ((t_builtin_fn)hash_get(&h, name));
}
