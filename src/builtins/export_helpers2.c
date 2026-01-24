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

/* helper: parse current argv element, consume following value,
	expand and dispatch */
static int	handle_parsed_export_arg(t_shell *st,
				t_vec av,
				int *ip,
				const char *argv0)
{
	int		i;
	char	*cur;
	char	*id;
	char	*val;
	char	quote;

	i = *ip;
	cur = ((char **)av.ctx)[i];
	id = NULL;
	val = NULL;
	parse_export_arg(cur, &id, &val);
	quote = strip_surrounding_quotes(&val);
	consume_following_value(av, &i, &val);
	*ip = i;
	if (val)
		val = expand_export_value(st, val, quote != '\'');
	return (handle_identifier(st, id, val, argv0));
}

/* reduced process_arg using helpers */
int	process_arg(t_shell *st, t_vec av, int *ip)
{
	int			i;
	char		*cur;
	const char	*arg0;

	i = *ip;
	arg0 = ((char **)av.ctx)[0];
	if (!av.ctx || i >= (int)av.len)
		return (verbose(CLAP_ERROR, ":[DEBUG export] missing "
				"argv element at index %d\n", i), 1);
	cur = ((char **)av.ctx)[i];
	return (handle_parsed_export_arg(st, av, ip, arg0));
	(void)cur;
}

/* handle identifier: set env or mark exported or print error
   signature reduced to 4 args: (state, id, val, argv0) */
int	handle_identifier(t_shell *st, char *id, char *val, const char *argv0)
{
	t_env	*e;

	if (ft_is_valid_ident(id))
	{
		if (!val)
		{
			e = env_get(&st->env, id);
			if (e)
				e->exported = true;
			free(id);
		}
		else
			env_set(&st->env, (t_env){true, id, val});
		return (0);
	}
	else
	{
		ft_eprintf("%s: %s: `%s' not valid identifier\n", st->context,
			argv0, id);
		return (free(id), free(val), 1);
	}
}
