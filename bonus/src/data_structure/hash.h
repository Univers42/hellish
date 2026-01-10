/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hash.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/10 00:48:35 by marvin            #+#    #+#             */
/*   Updated: 2026/01/10 00:48:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HASH_H
# define HASH_H

# include "../shell.h"
# include <stdlib.h>
# include <string.h>
# include <stdbool.h>
# include <stddef.h>

typedef struct s_hash_entry
{
	char	*key;
	void	*value;
}	t_hash_entry;

typedef struct s_vec t_hash; // ctx holds t_hash_entry

bool	hash_init(t_hash *h, size_t capacity);
size_t	hash_func(const char *key, size_t cap);
int		hash_find_idx(t_hash *h, const char *key);
bool	hash_resize(t_hash *h);
bool	hash_set(t_hash *h, const char *key, void *value);
void	*hash_get(t_hash *h, const char *key);
void	hash_destroy(t_hash *h, void (*free_val)(void *));

#endif