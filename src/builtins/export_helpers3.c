/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   export_helpers3.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* `export -n NAME...` -- take the export attribute away.
**
** The option was accepted (bad_opt_word lists 'n') and then ignored: the
** names went through the ordinary export path and came out MORE exported.
** Nothing could un-export a variable, because env_set overwrote the flag
** on every write and nothing ever asked it to clear one. It now keeps
** the flag (a plain assignment never un-exports, as in every shell), and
** this is the one place that takes it away, together with declare +x.
**
** `export -n NAME=value` assigns first, then un-exports, like bash. A
** name that does not exist is not an error (bash: status 0). */

/* True when one of the option words before the operands carries 'n'. */
bool	export_wants_unexport(t_vec av, size_t first_operand)
{
	size_t	i;
	char	*a;

	i = 1;
	while (i < first_operand && i < av.len)
	{
		a = ((char **)av.ctx)[i];
		if (a[0] == '-' && a[1] != '-' && ft_strchr(a, 'n'))
			return (true);
		i++;
	}
	return (false);
}

/* One operand of `export -n`: NAME or NAME=value. */
int	export_unexport_arg(t_shell *st, const char *word)
{
	char	*id;
	char	*val;

	id = NULL;
	val = NULL;
	parse_export_arg((char *)word, &id, &val);
	if (!id || !ft_is_valid_ident(id))
	{
		ft_eprintf("%s: export: `%s': not a valid identifier\n",
			st->ctx, word);
		xfree(id);
		xfree(val);
		return (1);
	}
	if (val)
		env_set(&st->env, env_create(ft_strdup(id), val, false));
	else
		xfree(val);
	env_unexport(&st->env, id);
	xfree(id);
	return (0);
}
