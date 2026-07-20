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

/* Populate the debug colour map with the second batch of token-type names.
   Split across two functions only because the norm limits function length;
   this half covers semicolons, newlines, and the quoted-word / env-var
   token types used by the VERBOSE expander dump. */
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

/* Pointer-identity sentinels for deferred positional tokens. When
   expansion defers $@ / $* to the splitter it points token.start at one
   of these static strings; the splitter recognises a deferred positional
   by POINTER equality (pos_mark('@') == token.start), which an expanded
   variable whose VALUE happens to be "@" can never satisfy. */
const char	*pos_mark(char which)
{
	static const char	at[2] = "@";
	static const char	star[2] = "*";

	if (which == '@')
		return (at);
	return (star);
}
