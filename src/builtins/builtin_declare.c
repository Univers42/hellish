/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_declare.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

/* declare / typeset: the subset scripts actually depend on.
   - declare -p [NAME...]  prints each variable's declaration in bash form
     (declare -a a=([0]="x") for arrays, declare -- v="val" for scalars).
   - declare [-aAgxr] NAME=VALUE  assigns (the type flags are accepted and
     mostly no-ops: arrays are auto-detected from () syntax, and -x export
     is honoured). Bare `declare NAME` just ensures the name is known.
   Type attributes beyond export are not tracked (a documented v1 limit);
   the point is that scripts using declare -a / declare -p run instead of
   dying on "command not found". */

/* Print one variable's declaration, bash-format. */
static int	declare_print_one(t_shell *state, const char *name)
{
	t_env	*e;
	char	*fmt;

	e = env_get(&state->env, (char *)name);
	if (!e)
		return (ft_eprintf("%s: declare: %s: not found\n",
				state->ctx, name), 1);
	if (arr_is(e->value))
	{
		fmt = arr_format(e->value);
		ft_printf("declare -a %s=%s\n", e->key, fmt);
		return (xfree(fmt), 0);
	}
	if (e->exported)
		ft_printf("declare -x %s=\"%s\"\n", e->key, e->value);
	else
		ft_printf("declare -- %s=\"%s\"\n", e->key, e->value);
	return (0);
}

/* declare -p: print the named variables (or, with none, all of them). */
static int	declare_print(t_shell *state, t_vec argv, size_t first)
{
	size_t	i;
	int		rc;

	if (first >= argv.len)
		return (set_print_env(state), 0);
	rc = 0;
	i = first;
	while (i < argv.len)
		if (declare_print_one(state, ((char **)argv.ctx)[i++]))
			rc = 1;
	return (rc);
}

/* One NAME or NAME=VALUE operand: NAME=VALUE assigns (export flag from
   -x), a bare NAME with no '=' is a no-op that just accepts the name. */
static void	declare_assign(t_shell *state, const char *word, int export)
{
	char	*eq;
	char	*key;

	eq = ft_strchr(word, '=');
	if (!eq)
	{
		if (export && env_get(&state->env, (char *)word))
			env_get(&state->env, (char *)word)->exported = true;
		return ;
	}
	key = ft_strndup(word, eq - word);
	env_set(&state->env, env_create(key, ft_strdup(eq + 1), export != 0));
}

int	builtin_declare(t_shell *state, t_vec argv)
{
	size_t	i;
	int		export;
	int		printmode;

	export = 0;
	printmode = 0;
	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		if (ft_strchr(((char **)argv.ctx)[i], 'p'))
			printmode = 1;
		if (ft_strchr(((char **)argv.ctx)[i], 'x'))
			export = 1;
		i++;
	}
	if (printmode)
		return (declare_print(state, argv, i));
	while (i < argv.len)
		declare_assign(state, ((char **)argv.ctx)[i++], export);
	return (0);
}
