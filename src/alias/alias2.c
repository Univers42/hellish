/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"
#include "libft.h"
#include <stdlib.h>

int	alias_remove(t_hash *aliases, const char *name)
{
	t_alias_entry	*e;

	e = (t_alias_entry *)hash_del(aliases, name);
	if (!e)
		return (1);
	xfree(e->name);
	xfree(e->value);
	xfree(e);
	return (0);
}

void	alias_print_all(t_hash *aliases)
{
	size_t			i;
	t_hash_entry	*entries;

	entries = (t_hash_entry *)aliases->ctx;
	i = 0;
	while (i < aliases->cap)
	{
		if (entries[i].key && entries[i].value)
		{
			ft_printf("alias %s='%s'\n",
				((t_alias_entry *)entries[i].value)->name,
				((t_alias_entry *)entries[i].value)->value);
		}
		i++;
	}
}

int	alias_print_one(t_hash *aliases, const char *name)
{
	char	*val;

	val = alias_get(aliases, name);
	if (!val)
		return (1);
	ft_printf("alias %s='%s'\n", name, val);
	return (0);
}
