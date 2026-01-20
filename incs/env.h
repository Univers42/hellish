/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 22:55:45 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 22:55:45 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ENV_H
# define ENV_H

# include "shell.h"
# include "helpers.h"

typedef struct s_env
{
	bool	exported;
	char	*key;
	char	*value;
}	t_env;

t_env		env_create(char *key, char *value, bool exported);
t_env		str_to_env(char *str);
t_vec_env	env_to_vec_env(t_shell *state, char **envp);
char		*env_expand_n(t_shell *state, char *key, int len);
char		*env_expand(t_shell *state, char *key);
void		env_extend(t_vec_env *dest, t_vec_env *src, bool export);
int			env_set(t_vec_env *v, t_env el);
t_env		*env_get(t_vec_env *env, char *key);
char		**get_envp(t_shell *state, char *exe_path);
t_env		*env_nget(t_vec_env *env, char *key, int len);
void		set_home(t_shell *state);
void		set_shlvl(t_shell *state);

static inline char	*env_get_ifs(t_vec_env *v)
{
	t_env	*e;

	e = env_get(v, "IFS");
	if (!e)
		return (" \t\n");
	return (e->value);
}

static inline int	env_len(char *line)
{
	int	len;

	len = 0;
	if (is_var_name_p1(line[len]))
		len++;
	else
		return (len);
	while (is_var_name_p2(line[len]))
		len++;
	return (len);
}

static inline void	free_env(t_vec_env *env)
{
	size_t	i;
	t_env	*curr;

	if (!env || env->ctx == NULL)
		return ;
	i = 0;
	while (i < env->len)
	{
		curr = &((t_env *)env->ctx)[i];
		free(curr->key);
		free(curr->value);
		i++;
	}
	free(env->ctx);
	env->ctx = NULL;
	env->len = 0;
	env->cap = 0;
	env->elem_size = sizeof(t_env);
}

#endif