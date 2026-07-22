/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_tokens.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 19:35:19 by marvin            #+#    #+#             */
/*   Updated: 2026/01/19 19:35:19 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "sys.h"

const char	*get_token_display_name(t_token *curr);

/* Print the lexeme text (with escape-sequence substitution) and pad with
   spaces to reach column width w_lexeme. Uses visible_lexeme_len to get
   the true display width so multi-byte or escaped chars are padded right. */
static void	print_token_lexeme(t_token *curr,
						size_t w_lexeme)
{
	size_t	vis;
	int		i;

	ft_printf("%s", ASCII_GREY);
	print_visible_lexeme_noquotes(curr);
	ft_printf("%s", RESET_TERM);
	vis = visible_lexeme_len(curr);
	i = -1;
	while (++i < (int)(w_lexeme - vis))
		ft_printf(" ");
}

/* For tokens with len == 0 (e.g. TT_END), print the EMPTY placeholder
   string and pad to column width so the table stays aligned. */
static void	print_token_empty(size_t w_lexeme)
{
	const char	*emp;
	size_t		vis;
	int			i;

	emp = EMPTY;
	vis = ft_strlen(emp);
	i = -1;
	ft_printf("%s%s%s", ASCII_GREY, emp, RESET_TERM);
	while (++i < (int)(w_lexeme - vis))
		ft_printf(" ");
}

/* Render one row of the token table: ║ type ║ len ║ lexeme ║. The type
   name is coloured per the colour map so operators, keywords and words are
   visually distinct even in a long token list. */
static void	print_token_row(t_token *curr,
						size_t w_name,
						size_t w_len,
						size_t w_lexeme)
{
	const char	*name;
	const char	*color;

	name = get_token_display_name(curr);
	color = token_color(curr->tt);
	ft_printf(ASCII_MAGENTA "║ " RESET_TERM);
	ft_printf("%s%-*s%s ", color, (int)w_name, name, RESET_TERM);
	ft_printf(ASCII_MAGENTA "║ " RESET_TERM);
	ft_printf("%*d ", (int)w_len, curr->len);
	ft_printf(ASCII_MAGENTA "║ " RESET_TERM);
	if (curr->len > 0)
		print_token_lexeme(curr, w_lexeme);
	else
		print_token_empty(w_lexeme);
	ft_printf(ASCII_MAGENTA " ║" RESET_TERM "\n");
}

/* Iterate over the full token deque and emit each row. The TT_END sentinel
   is included so we can confirm the deque is properly terminated. */
static void	print_token_rows(t_deque_tok *tokens,
						size_t w_name,
						size_t w_len,
						size_t w_lexeme)
{
	size_t	i;
	t_token	curr;

	i = -1;
	while (++i < tokens->deqtok.len)
	{
		curr = ltok2tok(*(t_ltoken *)deque_idx(&tokens->deqtok, i),
				tokens->base);
		print_token_row(&curr, w_name, w_len, w_lexeme);
	}
}

/* Pretty-print the token deque as a Unicode box-drawing table. Column widths
   are measured first so the table auto-fits the actual token content. Enable
   with VERBOSE / PRINT_TOKENS at compile or run time for debugging the lexer
   without changing any shell logic. */
void	print_tokens(t_deque_tok *tokens)
{
	size_t	w_name;
	size_t	w_len;
	size_t	w_lexeme;

	ft_printf("------- PRINTING TOKENS --------\n");
	compute_columns(tokens, &w_name, &w_len, &w_lexeme);
	print_table_header(w_name, w_len, w_lexeme);
	print_token_rows(tokens, w_name, w_len, w_lexeme);
	print_table_footer(w_name, w_len, w_lexeme);
	ft_printf("------- DONE --------\n");
}
