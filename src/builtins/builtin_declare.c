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

int	declare_assoc(t_shell *state, t_vec argv, size_t i);

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
	if (assoc_is(e->value))
	{
		fmt = assoc_format(e->value);
		ft_printf("declare -A %s=%s\n", e->key, fmt);
		return (xfree(fmt), 0);
	}
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

/* Scan declare's leading option words into a p/x/A bitmask (1/2/4).
   -F, -f, -n and -i are terminal: everything from that word on goes to the
   matching routine, so we stop at the word carrying one and report it
   through *term ('F' outranks 'f' the way bash's -F suppresses bodies;
   'n' outranks 'i' inside one cluster, as before). -F/-f used to match
   nothing here and were silently eaten as no-op options, which is why
   `declare -F` printed nothing and still exited 0 (issue #71 item 2).
   Returns the index of the first unconsumed word. */
static size_t	declare_scan(t_vec argv, int *flags, char *term)
{
	size_t	i;

	*flags = 0;
	*term = 0;
	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		if (ft_strchr(((char **)argv.ctx)[i], 'p'))
			*flags |= 1;
		if (ft_strchr(((char **)argv.ctx)[i], 'x'))
			*flags |= 2;
		if (ft_strchr(((char **)argv.ctx)[i], 'A'))
			*flags |= 4;
		*term = scan_term(((char **)argv.ctx)[i]);
		if (*term)
			return (i);
		i++;
	}
	return (i);
}

int	builtin_declare(t_shell *state, t_vec argv)
{
	size_t	i;
	int		flags;
	char	term;

	i = declare_scan(argv, &flags, &term);
	if (term == 'F' || term == 'f')
		return (declare_functions(state, argv, i + 1, term == 'f'));
	if (term == 'n')
		return (declare_nameref(state, argv, i));
	if (term == 'i')
		return (declare_integer(state, argv, i));
	if (flags & 4)
		return (declare_assoc(state, argv, i));
	if (flags & 1)
		return (declare_print(state, argv, i));
	while (i < argv.len)
		declare_assign(state, ((char **)argv.ctx)[i++], (flags & 2) != 0);
	return (0);
}
