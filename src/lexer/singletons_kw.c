/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   singletons_kw.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

/* Keyword token names -- part 1: the if/while/until/for/do/done family.
   Called from get_tt_names() during one-time initialisation. */
void	init_tt_names_kw1(const char **names)
{
	names[TT_IF] = "TT_IF";
	names[TT_THEN] = "TT_THEN";
	names[TT_ELIF] = "TT_ELIF";
	names[TT_ELSE] = "TT_ELSE";
	names[TT_FI] = "TT_FI";
	names[TT_WHILE] = "TT_WHILE";
	names[TT_UNTIL] = "TT_UNTIL";
	names[TT_FOR] = "TT_FOR";
	names[TT_DO] = "TT_DO";
	names[TT_DONE] = "TT_DONE";
}

/* Keyword token names -- part 2: case/esac/in, braces, ! and ;;. */
void	init_tt_names_kw2(const char **names)
{
	names[TT_CASE] = "TT_CASE";
	names[TT_ESAC] = "TT_ESAC";
	names[TT_IN] = "TT_IN";
	names[TT_LBRACE] = "TT_LBRACE";
	names[TT_RBRACE] = "TT_RBRACE";
	names[TT_BANG] = "TT_BANG";
	names[TT_DSEMI] = "TT_DSEMI";
	names[TT_SELECT] = "TT_SELECT";
}

/* Map all keyword token names to the magenta colour so they stand out in the
   debug token table. This is intentionally uniform -- one colour for all
   keywords makes it easy to spot structural tokens at a glance. */
void	init_color_map_kw(t_hash *map)
{
	hash_set(map, "TT_IF", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_THEN", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_ELIF", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_ELSE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_FI", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_WHILE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_UNTIL", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_FOR", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_DO", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_DONE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_CASE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_ESAC", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_IN", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_LBRACE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_RBRACE", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_BANG", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_DSEMI", (void *)ASCII_MAGENTA);
	hash_set(map, "TT_SELECT", (void *)ASCII_MAGENTA);
}
