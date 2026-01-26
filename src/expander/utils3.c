/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils3.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/26 02:46:49 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/26 02:53:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

void	init_color_map_part2(t_hash *map)
{
	hash_set(map, "TT_SEMICOLON", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_NEWLINE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_QWORD", (void *)ASCII_GREEN);
	hash_set(map, "TT_SQWORD", (void *)ASCII_GREEN);
	hash_set(map, "TT_DQWORD", (void *)ASCII_GREEN);
	hash_set(map, "TT_ENVVAR", (void *)ASCII_GREEN);
	hash_set(map, "TT_DQENVVAR", (void *)ASCII_GREEN);
}
