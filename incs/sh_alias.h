/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sh_alias.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Alias subsystem public types and API.
   Aliases are stored in a t_hash keyed on the alias name.  Expansion
   happens at the input level (before lexing) so aliases work identically
   to bash: they replace the first word of a simple command.  Recursive
   expansion is bounded by ALIAS_MAX_DEPTH to prevent infinite loops. */

#ifndef SH_ALIAS_H
# define SH_ALIAS_H

# include "libft.h"

/* Maximum alias expansion recursion depth (alias a='b'; alias b='a').
   64 matches the bash limit; beyond this we stop expanding and leave
   the string as-is rather than looping forever. */
# define ALIAS_MAX_DEPTH 64

/* One alias definition.  Both strings are heap-allocated and owned
   by this entry (freed in alias_remove / alias_table_free). */
typedef struct s_alias_entry
{
	char	*name; /* the alias name (e.g. "ll") */
	char	*value; /* the replacement text (e.g. "ls -la") */
}	t_alias_entry;

struct	s_shell;

void	alias_table_init(t_hash *aliases);
void	alias_table_free(t_hash *aliases);
int		alias_set(t_hash *aliases, const char *name, const char *value);
char	*alias_get(t_hash *aliases, const char *name);
int		alias_remove(t_hash *aliases, const char *name);
void	alias_print_all(t_hash *aliases);
int		alias_print_one(t_hash *aliases, const char *name);
char	*alias_expand_input(t_hash *aliases, const char *input);

#endif
