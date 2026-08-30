/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cmd_hash.h                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CMD_HASH_H
# define CMD_HASH_H

# include "libft.h"

typedef struct s_cmd_hash_entry
{
	char	*name;
	char	*path;
	int		hits;
}	t_cmd_hash_entry;

struct	s_shell;

void	cmd_hash_init(t_hash *ht);
void	cmd_hash_free(t_hash *ht);
char	*cmd_hash_lookup(t_hash *ht, const char *name);
void	cmd_hash_insert(t_hash *ht,
			const char *name,
			const char *path);
void	cmd_hash_remove(t_hash *ht, const char *name);
void	cmd_hash_clear(t_hash *ht);
void	cmd_hash_print_all(t_hash *ht);
void	cmd_hash_print_reusable(t_hash *ht);

#endif
