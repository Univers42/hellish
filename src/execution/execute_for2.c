/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_for2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"

/* Snapshot the positional parameters into an owned char* vector. POSIX:
   `for x do ...` iterates the expansion of "$@" computed ONCE at loop
   entry — a `shift` or `set` inside the body (autoconf's config.status
   does both) must not change the iteration list, and reading $N lazily
   against a stale $# dereferenced NULL. */
void	snapshot_positionals(t_shell *state, t_vec *out)
{
	const char	*raw;
	char		*key;
	char		*val;
	int			n;
	int			i;

	vec_init(out);
	out->elem_size = sizeof(char *);
	n = ft_atoi(env_expand(state, "#"));
	i = 0;
	while (++i <= n)
	{
		key = ft_itoa(i);
		raw = env_expand(state, key);
		xfree(key);
		if (raw)
			val = ft_strdup(raw);
		else
			val = ft_strdup("");
		vec_push(out, &val);
	}
}

/* Free a snapshot built by snapshot_positionals. */
void	free_positional_snapshot(t_vec *w)
{
	size_t	i;

	i = 0;
	while (i < w->len)
		xfree(((char **)w->ctx)[i++]);
	xfree(w->ctx);
	*w = (t_vec){0};
}

/* Assign the loop variable for one for-loop iteration.  If the variable
   already exists we update it in place (swap the value string, reuse the
   env entry) to avoid creating duplicate entries.  If it is new we create
   a non-exported entry; it will become exported only if allexport is set
   via env_set (which calls env_check_export). */
void	set_for_var(t_shell *state, char *name, char *val)
{
	t_env	*old;

	old = env_get(&state->env, name);
	if (old)
	{
		xfree(old->value);
		old->value = ft_strdup(val);
		return ;
	}
	env_set(&state->env,
		env_create(ft_strdup(name), ft_strdup(val), false));
}

/* Release the expanded word list: the strings were strdup'd by the expander
   and the backing array is plain heap. */
void	free_word_vec(t_vec *w)
{
	size_t	i;

	i = 0;
	while (i < w->len)
		xfree(((char **)w->ctx)[i++]);
	xfree(w->ctx);
}
