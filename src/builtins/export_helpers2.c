/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   export.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:29:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:29:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "helpers.h"

/* Full processing for one export argument: parse name=value, strip quotes,
   expand (unless single-quoted), then delegate to handle_identifier to set
   or mark-export the variable.

   Each operand stands alone. There is deliberately no `export NAME value`
   two-word form: bash has none, and inventing one made `export A B C` read
   its operands pairwise -- assigning B as A's value and never exporting B
   at all (GitHub issue #12). A bare NAME must leave the value untouched. */
static int	handle_parsed_export_arg(t_shell *st,
				t_vec av,
				int i,
				const char *argv0)
{
	char	*id;
	char	*val;
	char	quote;

	id = NULL;
	val = NULL;
	parse_export_arg(((char **)av.ctx)[i], &id, &val);
	quote = strip_surrounding_quotes(&val);
	if (val)
		val = expand_export_value(st, val, quote != '\'');
	return (handle_identifier(st, id, val, argv0));
}

/* Process one element of the export argv. Bounds-checks `i` first so we
   never read past the end of av. */
int	process_arg(t_shell *st, t_vec av, int i)
{
	const char	*arg0;

	arg0 = ((char **)av.ctx)[0];
	if (!av.ctx || i >= (int)av.len)
		return (verbose(CLAP_ERROR, ":[DEBUG export] missing "
				"argv element at index %d\n", i), 1);
	return (handle_parsed_export_arg(st, av, i, arg0));
}

/* Do the actual env work for one export argument. If `id` is a valid POSIX
   identifier: with a value, env_set marks it exported and stores the value;
   without a value (bare `export NAME`), find the existing entry and set its
   exported flag. Invalid identifiers print an error, free both strings, and
   return 1 — but do NOT abort the loop in process_arg. */
/* `export NAME+=value` (bash append): the parser handed us id="NAME+" and
   val="value".  Strip the trailing '+' and splice the current value of NAME
   in front of val, so `export a=1; export a+=2` leaves a="12".  A no-op when
   id does not end in '+'. */
static char	*export_apply_append(t_shell *st, char *id, char *val)
{
	size_t	n;
	t_env	*e;
	char	*old;
	char	*joined;

	n = ft_strlen(id);
	if (n == 0 || id[n - 1] != '+' || !val)
		return (val);
	id[n - 1] = '\0';
	e = env_get(&st->env, id);
	old = "";
	if (e && e->value)
		old = e->value;
	joined = ft_strjoin(old, val);
	xfree(val);
	return (joined);
}

int	handle_identifier(t_shell *st, char *id, char *val, const char *argv0)
{
	t_env	*e;

	val = export_apply_append(st, id, val);
	if (ft_is_valid_ident(id))
	{
		if (!val)
		{
			e = env_get(&st->env, id);
			if (e)
				e->exported = true;
			xfree(id);
		}
		else
			env_set(&st->env, (t_env){true, id, val});
		return (0);
	}
	else
	{
		ft_eprintf("%s: %s: `%s' not valid identifier\n", st->ctx,
			argv0, id);
		return (xfree(id), xfree(val), 1);
	}
}
