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

void	print_table_header(size_t w_name, size_t w_len, size_t w_lexeme)
{
	print_table_header_top(w_name, w_len, w_lexeme);
	print_table_header_titles(w_name, w_len, w_lexeme);
	print_table_header_mid(w_name, w_len, w_lexeme);
}
