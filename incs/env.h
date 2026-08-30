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
# include <limits.h>

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
/* Associative array (declare -A): same "key<US>value" records joined by
   RS, but a distinct magic so the shell knows the subscript is a LITERAL
   string (not an arithmetic index) and records are NOT numerically
   sorted. The attribute lives in the value — an empty assoc array is
   just ARR_ASSOC_MAGIC — so no per-variable attribute table is needed. */
# define ARR_ASSOC_MAGIC '\x1c'

/* A contiguous run of elements, 0-BASED and INCLUSIVE, as the store counts
** them.  zsh's `a[lo,hi]` is the only thing that produces one; the dialect's
** 1-based, negative-wrapping subscripts are resolved into this shape by
** zsh_slice_bounds(), so nothing below the expander has to know which
** dialect is running.
**
** hi < lo is an EMPTY range and is meaningful rather than an error: reading
** it gives "", and assigning to it INSERTS at lo without removing anything.
** `a=(1 2 3); a[3,2]=(P)` is how zsh splices P in ahead of element 3.
**
** lo == SLICE_NONE means "not a slice at all" -- the subscript was an
** ordinary arithmetic index, or the zsh dialect is not in effect.
*/
# define SLICE_NONE LONG_MIN

typedef struct s_slice
{
	long	lo;
	long	hi;
}	t_slice;

bool		arr_is(const char *val);
bool		arr_next(const char **cur, long *idx, const char **v, int *vl);
int			arr_count(const char *val);
char		*arr_get_idx(const char *val, long want);
char		*arr_join(const char *val, char sep);
char		*arr_join_range(const char *val, char sep, t_slice r);
char		*arr_with_set(const char *old, long idx, const char *v);
char		*arr_from_elems(char **elems, int n, const char *base);
char		*arr_format(const char *val);
char		*arr_without(const char *old, long idx);
char		*arr_splice(const char *old, t_slice r, char **elems, int n);
long		arr_max_idx(const char *val);
void		rec_append(t_string *out, long idx, const char *v, int vl);

/* Associative arrays (env_assoc*.c). */
/* declare -i / -n attribute table (env_attr.c). One entry per attributed
   variable; the table is normally empty, so lookups short-circuit and the
   assignment/read hot paths pay nothing. */
typedef struct s_var_attr
{
	char	*name;
	char	kind; /* 'i' integer, 'n' nameref */
	char	*target; /* nameref target name (NULL for -i) */
}	t_var_attr;

char		attr_kind(t_shell *state, const char *name, int nlen);
char		*attr_target(t_shell *state, const char *name, int nlen);
void		attr_set(t_shell *state, const char *name, char kind,
				const char *target);
void		attr_clear(t_shell *state);

/* Streaming iterator over an assoc-encoded value: prime with
   assoc_it_init, then each assoc_next fills k/kl (key slice) and
   v/vl (value slice), false at end. One struct instead of the five
   pointer out-params the 42 norm forbids. */
typedef struct s_assoc_it
{
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;
}	t_assoc_it;

bool		assoc_is(const char *val);
void		assoc_it_init(t_assoc_it *it, const char *val);
bool		assoc_next(t_assoc_it *it);
int			assoc_count(const char *val);
char		*assoc_get(const char *val, const char *key, int klen);
char		*assoc_with_set(const char *old, const char *key, int klen,
				const char *nv);
char		*assoc_without(const char *old, const char *key, int klen);
char		*assoc_keys(const char *val);
char		*assoc_values(const char *val, char sep);
char		*assoc_format(const char *val);
void		vec_push_dquoted(t_string *out, const char *s, int len);
bool		assoc_key_quoted(const char *k, int len);
char		*dquote_str(const char *s);
void		env_extend(t_vec_env *dest, t_vec_env *src, bool export);
int			env_set(t_vec_env *v, t_env el);
t_env		*env_get(t_vec_env *env, char *key);
char		**get_envp(t_shell *state, char *exe_path);
char		**get_envp_all(t_shell *state, char *exe_path);
char		*env_to_str(t_env *e);
t_env		*env_nget(t_vec_env *env, char *key, int len);
void		set_shlvl(t_shell *state);
void		set_shell_var(t_shell *state);

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