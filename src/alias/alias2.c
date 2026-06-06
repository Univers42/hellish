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

/* alias remove + print helpers (unalias and `alias` builtins).
   alias_print_all iterates the raw hash bucket array, skipping empty
   slots.  Output format matches bash: "alias name='value'". */

#include "sh_alias.h"
#include "libft.h"
#include <stdlib.h>

/* Remove the named alias from the table, freeing all memory.
   Returns 0 on success, 1 if the alias did not exist (unalias -a
   should not error on missing names per POSIX). */
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

/* Print every defined alias, one per line.  We walk the raw bucket array
   because there is no iterator API; empty slots have key==NULL. */
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
