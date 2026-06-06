/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tables.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:06:38 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:06:38 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "libft.h"
#include <stdio.h>
#include <stdlib.h>

/* Draw the bottom border of the token debug table using box-drawing chars.
   Each column is padded to width w_name/w_len/w_lexeme + 2 (for spaces). */
void	print_table_footer(size_t w_name, size_t w_len, size_t w_lexeme)
{
	int	i;

	ft_printf(ASCII_MAGENTA "╚");
	i = -1;
	while (++i < (int)w_name + 2)
		ft_printf("═");
	ft_printf("╩");
	i = -1;
	while (++i < (int)w_len + 2)
		ft_printf("═");
	ft_printf("╩");
	i = -1;
	while (++i < (int)w_lexeme + 2)
		ft_printf("═");
	ft_printf("╝\n" RESET_TERM);
}

/* Top border: ╔═══╦═══╦═══╗ using Unicode box-drawing. */
static void	print_table_header_top(size_t w_name, size_t w_len, size_t w_lexeme)
{
	int	i;

	ft_printf(ASCII_MAGENTA "╔");
	i = -1;
	while (++i < (int)w_name + 2)
		ft_printf("═");
	ft_printf("╦");
	i = -1;
	while (++i < (int)w_len + 2)
		ft_printf("═");
	ft_printf("╦");
	i = -1;
	while (++i < (int)w_lexeme + 2)
		ft_printf("═");
	ft_printf("╗\n" RESET_TERM);
}

/* Column headings row: ║ type ║ len ║ lexeme ║, bold-formatted. */
static void	print_table_header_titles(size_t w_name,
								size_t w_len,
								size_t w_lexeme)
{
	ft_printf(ASCII_MAGENTA "║ " RESET_TERM BOLD_TERM "%-*s" RESET_TERM
		" " ASCII_MAGENTA "║ " RESET_TERM BOLD_TERM "%*s" RESET_TERM
		" " ASCII_MAGENTA "║ " RESET_TERM BOLD_TERM
		"%-*s" RESET_TERM " " ASCII_MAGENTA "║\n" RESET_TERM,
		(int)w_name, "type", (int)w_len, "len", (int)w_lexeme, "lexeme");
}

/* Separator between the headings row and the data rows: ╠═══╬═══╬═══╣. */
static void	print_table_header_mid(size_t w_name, size_t w_len, size_t w_lexeme)
{
	int	i;

	ft_printf(ASCII_MAGENTA "╠");
	i = -1;
	while (++i < (int)w_name + 2)
		ft_printf("═");
	ft_printf("╬");
	i = -1;
	while (++i < (int)w_len + 2)
		ft_printf("═");
	ft_printf("╬");
	i = -1;
	while (++i < (int)w_lexeme + 2)
		ft_printf("═");
	ft_printf("╣\n" RESET_TERM);
}

/* Compose the full table header (top border + titles + mid separator)
   from the three helper functions above. Column widths are pre-computed
   by compute_columns so the table fits the actual token data exactly. */
void	print_table_header(size_t w_name, size_t w_len, size_t w_lexeme)
{
	print_table_header_top(w_name, w_len, w_lexeme);
	print_table_header_titles(w_name, w_len, w_lexeme);
	print_table_header_mid(w_name, w_len, w_lexeme);
}
