/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/06 22:54:40 by marvin            #+#    #+#             */
/*   Updated: 2026/01/06 22:54:40 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUILTINS_H
# define BUILTINS_H

# include "common.h"

typedef struct s_ctx_builtin
{
	t_string	cwd;
	char		*context;
	t_env		env;
	char		*pid;
}	t_ctx_builtin;

typedef int	(*t_builtin_fn)(t_ctx_builtin *state, t_strings argv);

/* Forward declarations only — implementations are in builtins.c */
int	builtin_echo(t_ctx_builtin *state, t_strings argv);
int	builtin_export(t_ctx_builtin *state, t_strings argv);
int	builtin_cd(t_ctx_builtin *state, t_strings argv);
int	builtin_exit(t_ctx_builtin *state, t_strings argv);
int	builtin_pwd(t_ctx_builtin *state, t_strings argv);
int	builtin_env(t_ctx_builtin *state, t_strings argv);
int	builtin_unset(t_ctx_builtin *state, t_strings argv);

/* Initialize builtin function dispatch table. */
static inline t_hash	*builtins_init(void)
{
	t_hash	*dispatch;

	dispatch = malloc(sizeof(t_hash));
	if (dispatch == NULL)
		return (NULL);
	if (!hash_init(dispatch, 16))
	{
		free(dispatch);
		return (NULL);
	}
	hash_set(dispatch, "echo", (void *)builtin_echo);
	hash_set(dispatch, "export", (void *)builtin_export);
	hash_set(dispatch, "cd", (void *)builtin_cd);
	hash_set(dispatch, "exit", (void *)builtin_exit);
	hash_set(dispatch, "pwd", (void *)builtin_pwd);
	hash_set(dispatch, "env", (void *)builtin_env);
	hash_set(dispatch, "unset", (void *)builtin_unset);
	return (dispatch);
}

/* Get builtin function by name. */
static inline t_builtin_fn	builtin_func(t_hash *dispatch, char *name)
{
	if (!name || !dispatch)
		return (NULL);
	return ((t_builtin_fn)hash_get(dispatch, name));
}

#endif
