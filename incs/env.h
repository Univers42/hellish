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

/* Public API for the shell environment subsystem.
   The environment is a t_vec_env (a growable array of t_env) rather than
   the raw `char **environ`.  This gives us O(1) lookup (via the index in
   env_index.c), exported/non-exported distinction, and clean ownership.
   Key rule: t_env OWNS its key and value strings; never alias them. */

#ifndef ENV_H
# define ENV_H

# include "shell.h"
# include "helpers.h"

/* One shell variable.  exported=true means it will appear in the envp[]
   passed to execve (i.e. child processes see it).  exported=false makes
   it a shell-local variable, invisible to children. */
typedef struct s_env
{
	bool	exported; /* true => visible to child processes via execve */
	char	*key; /* heap-allocated variable name (owned) */
	char	*value; /* heap-allocated value string (owned) */
}	t_env;

t_env		env_create(char *key, char *value, bool exported);
t_env		str_to_env(char *str);
t_vec_env	env_to_vec_env(t_shell *state, char **envp);
char		*env_expand_n(t_shell *state, char *key, int len);
char		*env_expand(t_shell *state, char *key);

/* Indexed arrays (env_array*.c). An array VALUE is an ordinary env string
   with a private encoding: ARR_MAGIC, then "index<ARR_US>value" records
   joined by ARR_RS, kept sorted by index (sparse indices supported).
   Nothing in the env lifecycle (set/copy/free/fork) knows or cares — the
   encoding is invisible outside the array helpers, the expander, and the
   two listing sites that pretty-print it. Arrays are never exported to
   execve (get_envp skips them), matching bash. */
# define ARR_MAGIC '\x1d'
# define ARR_RS '\x1e'
# define ARR_US '\x1f'

bool		arr_is(const char *val);
bool		arr_next(const char **cur, long *idx, const char **v, int *vl);
int			arr_count(const char *val);
char		*arr_get_idx(const char *val, long want);
char		*arr_join(const char *val, char sep);
char		*arr_with_set(const char *old, long idx, const char *v);
char		*arr_from_elems(char **elems, int n, const char *base);
char		*arr_format(const char *val);
void		env_extend(t_vec_env *dest, t_vec_env *src, bool export);
int			env_set(t_vec_env *v, t_env el);
t_env		*env_get(t_vec_env *env, char *key);
char		**get_envp(t_shell *state, char *exe_path);
char		**get_envp_all(t_shell *state, char *exe_path);
char		*env_to_str(t_env *e);
t_env		*env_nget(t_vec_env *env, char *key, int len);
void		set_shlvl(t_shell *state);

/* O(1) env name index (env_index.c) -- lazy hash table over the vector.
   Pass len<0 to use strlen(key).  Returns vector position, -1 if absent. */
int			env_index_find(t_vec_env *env, const char *key, int len);
void		env_index_add(t_vec_env *env, int idx);
void		env_index_mark_dirty(void);
void		env_index_free(void);

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
		xfree(curr->key);
		xfree(curr->value);
		i++;
	}
	xfree(env->ctx);
	env->ctx = NULL;
	env->len = 0;
	env->cap = 0;
	env->elem_size = sizeof(t_env);
}

#endif