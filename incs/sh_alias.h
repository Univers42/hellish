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

#ifndef SH_ALIAS_H
# define SH_ALIAS_H

# include "libft.h"

# define ALIAS_MAX_DEPTH 64

typedef struct s_alias_entry
{
	char	*name;
	char	*value;
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
