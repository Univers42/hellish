/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 22:40:02 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 22:40:02 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Shell init helpers: cwd tracking, HOME/SHLVL seeding, and the fast
   env_nget path.  These run once at startup -- correctness > speed here.
   The tricky one is set_shlvl: we parse the parent's value with a cap of
   42 (the school limit) to avoid exploding SHLVL on deeply-nested shells
   that someone ssh'd into 200 times. */

#include "shell.h"
#include "env.h"
#include "libft.h"
#include "sys.h"

/* Seed state->cwd from getcwd().  If the syscall fails (deleted dir,
   permissions) we emit a warning and leave cwd empty rather than crash.
   The vec_push_str copies the string, so the getcwd buffer is freed. */
void	init_cwd(t_shell *state)
{
	char	*cwd;

	vec_init(&state->cwd);
	cwd = x_getcwd();
	if (cwd)
		vec_push_str(&state->cwd, cwd);
	else
		ft_eprintf(MSG_GETCWD_SHINIT);
	xfree(cwd);
}

/* Ensure HOME is set.  If the inherited env lacks it (rare but possible
   with stripped environments), fall back to the current working dir, and
   if even that fails, to /tmp. Worst case the user can still `cd`. */
void	set_home(t_shell *state)
{
	t_env	*e;
	char	*cwd;

	e = env_get(&state->env, HOME);
	if (!e || !e->value || !e->value[0])
	{
		cwd = x_getcwd();
		if (!cwd)
			cwd = ft_strdup(TMP_DIR);
		env_set(&state->env, env_create(ft_strdup(HOME),
				ft_strdup(cwd), true));
		xfree(cwd);
	}
}

/* Initialise the cwd vec and immediately push the actual path into it.
   The double-push (init_cwd already pushes once) looks redundant but
   init_cwd is called separately by subshell init paths that don't call
   set_cwd; keep them decoupled. */
void	set_cwd(t_shell *state)
{
	char	*cwd;

	init_cwd(state);
	cwd = x_getcwd();
	if (cwd)
		vec_push_str(&state->cwd, cwd);
	else
		ft_eprintf(MSG_GETCWD_SHINIT);
	xfree(cwd);
}

/* Increment SHLVL (capped via ft_checked_atoi's flags=42 to avoid huge
   values from weird nesting).  Missing or non-numeric -> treat as 1. */
void	set_shlvl(t_shell *state)
{
	t_env	*e;
	int		lvl;
	char	buf[16];

	e = env_get(&state->env, SHLVL);
	if (!e || !e->value || !e->value[0])
	{
		env_set(&state->env, env_create(ft_strdup(SHLVL),
				ft_strdup("1"), true));
	}
	else
	{
		lvl = 1;
		if (ft_checked_atoi(e->value, &lvl, 42) == 0)
			lvl++;
		else
			lvl = 1;
		ft_snprintf(buf, sizeof(buf), "%d", lvl);
		env_set(&state->env, env_create(ft_strdup(SHLVL),
				ft_strdup(buf), true));
	}
}

/* Fast env lookup by key with explicit length (avoids a strlen when the
   caller already knows it).  Returns a pointer into the vector -- valid
   until the next env_set/env_unset that could trigger a realloc. */
t_env	*env_nget(t_vec_env *env, char *key, int len)
{
	int	idx;

	idx = env_index_find(env, key, len);
	if (idx < 0)
		return (0);
	return (&((t_env *)env->ctx)[idx]);
}
