/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_help.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "builtins_private.h"
#include "help.h"

/* One `help NAME`. Returns 0 when the topic exists, 1 otherwise -- and the
   "not found" goes to stderr with a non-zero status, because `help foo` in
   a script is a question that was answered wrongly, not output. */
static int	help_one(const char *name, int synopsis_only, t_shell *state)
{
	const t_help	*e;

	e = help_find(name);
	if (!e)
	{
		ft_eprintf("%s: help: no help topics match `%s'. "
			"Try `help help'.\n", state->ctx, name);
		return (1);
	}
	help_print_one(e, synopsis_only);
	return (0);
}

/* help [-s] [topic ...]: with no topic, the grouped listing; with topics,
   each one in turn.

   The status is bash's rule, measured rather than invented: 0 when AT LEAST
   ONE topic matched, 1 only when none did. `help cd nosuch` is 0 in bash.
   I would have picked "1 if any topic was unknown" as the more truthful
   answer, but error wording is free here and exit codes are not -- a
   gratuitous divergence in a status is exactly the kind of thing that
   breaks somebody's script for no benefit. */
int	builtin_help(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		synopsis_only;
	int		found;

	av = (char **)argv.ctx;
	i = 1;
	synopsis_only = (argv.len > 1 && !ft_strcmp(av[1], "-s"));
	i += synopsis_only;
	if (i >= argv.len)
	{
		help_print_list();
		return (0);
	}
	found = 0;
	while (i < argv.len)
		found |= !help_one(av[i++], synopsis_only, state);
	return (!found);
}
