/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_dft_env.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:03:54 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 00:03:54 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Called once during shell init to fill in any missing POSIX-mandatory
   env vars.  The pattern for each variable is identical: check if already
   set with a non-empty value; if not, synthesise a sensible default.
   We never override what the user's parent shell gave us -- only patch
   the gaps. */

#include "shell.h"
#include "env.h"
#include "libft.h"
#include "sys.h"

/* If PATH is missing, seed it with DFT_PATH (a macro with the usual
   /usr/local/bin:/usr/bin:/bin colon-list).  A stripped env can otherwise
   make even `ls` fail with command-not-found. */
void	set_path(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, PATH);
	if (!e || !e->value || !e->value[0])
	{
		env_set(&state->env, env_create(ft_strdup(PATH),
				ft_strdup(DFT_PATH), true));
	}
}

/* Seed $_ (the last argument of the previous command, or the shell name
   on startup).  POSIX leaves the initial value implementation-defined;
   bash sets it to the shell path, so we do too. */
void	set_underscore(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, ULTIMATE_ARG);
	if (!e || !e->value || !e->value[0])
	{
		if (state->ctx)
			env_set(&state->env, env_create(ft_strdup(ULTIMATE_ARG),
					ft_strdup(state->ctx), true));
		env_set(&state->env, env_create(ft_strdup(ULTIMATE_ARG),
				ft_strdup(MINISHELL), true));
	}
}

/* getopts state starts fresh in every shell: bash initialises OPTIND
   to 1 on startup, overriding whatever the parent exported (an inherited
   `OPTIND=7` still shows up as 1) while keeping the exported flag so
   children continue to see the variable. A bare `echo $OPTIND` in a
   fresh shell must print 1. */
static void	set_optind(t_shell *state)
{
	t_env	*e;

	e = env_get(&state->env, "OPTIND");
	if (e)
		env_set(&state->env, env_create(ft_strdup("OPTIND"),
				ft_strdup("1"), e->exported));
	else
		env_set(&state->env, env_create(ft_strdup("OPTIND"),
				ft_strdup("1"), false));
}

/* Identity variables bash sets as (non-exported) shell vars at startup:
   $UID, $HOSTNAME, $OSTYPE. Only patched in when the parent left a gap,
   like every other setter here. OSTYPE matches bash's configure triplet
   suffix on this platform. */
static void	set_id_vars(t_shell *state)
{
	char	host[256];

	if (!env_get(&state->env, "UID"))
		env_set(&state->env, env_create(ft_strdup("UID"),
				ft_itoa((int)getuid()), false));
	if (!env_get(&state->env, "EUID"))
		env_set(&state->env, env_create(ft_strdup("EUID"),
				ft_itoa((int)geteuid()), false));
	if (!env_get(&state->env, "HOSTNAME")
		&& gethostname(host, sizeof(host)) == 0)
		env_set(&state->env, env_create(ft_strdup("HOSTNAME"),
				ft_strdup(host), false));
	if (!env_get(&state->env, "OSTYPE"))
		env_set(&state->env, env_create(ft_strdup("OSTYPE"),
				ft_strdup("linux-gnu"), false));
	if (!env_get(&state->env, "BASH_VERSION"))
		env_set(&state->env, env_create(ft_strdup("BASH_VERSION"),
				ft_strdup("5.2.0(1)-release"), false));
	if (!env_get(&state->env, "BASH_VERSINFO"))
		env_set(&state->env, env_create(ft_strdup("BASH_VERSINFO"),
				arr_from_elems((char *[]){"5", "2", "0", "1",
					"release", "x86_64-pc-linux-gnu"}, 6, NULL), false));
}

/* Top-level init: call all the individual setters in order, then add
   PPID (read-only by convention but not enforced) and PWD if absent.
   Call this AFTER env_to_vec_env so the env vector already exists. */
void	ensure_essential_env_vars(t_shell *state)
{
	char	*cwd;
	t_env	*e;

	cwd = NULL;
	set_path(state);
	set_shlvl(state);
	set_shell_var(state);
	set_underscore(state);
	set_optind(state);
	set_id_vars(state);
	env_set(&state->env, env_create(ft_strdup("PPID"),
			ft_itoa((int)getppid()), false));
	e = env_get(&state->env, PWD);
	if (!e || !e->value || !e->value[0])
	{
		cwd = x_getcwd();
		if (!cwd)
			cwd = ft_strdup(TMP_DIR);
		env_set(&state->env, env_create(ft_strdup(PWD),
				ft_strdup(cwd), true));
		xfree(cwd);
	}
}
