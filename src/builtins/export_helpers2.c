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

/* reduced process_arg using helpers */
int process_arg(t_shell *st, t_vec av, int *ip)
{
	char	*id;
	char	*val;
	int		i;
	char	*arg0;
	char	*cur;
	char	quote;

	i = *ip;
	arg0 = ((char **)av.ctx)[0];
	cur = ((char **)av.ctx)[i];
	if (!av.ctx || !cur)
		return (ft_eprintf("[DEBUG export] missing argv element at index %d\n", i), 1);
	parse_export_arg(cur, &id, &val);
	quote = strip_surrounding_quotes(&val);
	consume_following_value(av, &i, &val);
	*ip = i;
	if (val)
		val = expand_export_value(st, val, quote != '\'');
	return (handle_identifier(st, id, val, arg0, cur));
}

/* handle identifier: set env or mark exported or print error */
int	handle_identifier(t_shell *st, char *id, char *val, const char *argv0, const char *orig_arg)
{
	t_env *e;

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
		return (ft_eprintf("%s: %s: `%s' not valid identifier\n", st->context,
			argv0, orig_arg),
		free(id), free(val), 1);
}
