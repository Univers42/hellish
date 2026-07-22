/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   print_header.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/19 19:56:11 by marvin            #+#    #+#             */
/*   Updated: 2026/01/19 19:56:11 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "sys.h"

/* Walk every token in the deque and track the maximum string widths for the
   three debug-table columns (type name, decimal length, visible lexeme). We
   seed with minimums (4/"len"/ft_strlen(EMPTY)) so single-token inputs still
   produce a readable table. The lexeme width uses visible_lexeme_len, which
   accounts for escape sequences like `\n` rendered as two chars. */
void	compute_columns(t_deque_tok *tokens, size_t *w_name,
		size_t *w_len, size_t *w_lexeme)
{
	size_t	i;
	size_t	n;
	t_token	t;
	size_t	tmp;

	n = tokens->deqtok.len;
	*w_name = 4;
	*w_len = 3;
	*w_lexeme = ft_strlen(EMPTY);
	i = -1;
	while (++i < n)
	{
		t = ltok2tok(*(t_ltoken *)deque_idx(&tokens->deqtok, i), tokens->base);
		tmp = ft_strlen(tt_to_str(t.tt));
		if (tmp > *w_name)
			*w_name = tmp;
		tmp = 0;
		if (t.len > 0)
			tmp = visible_lexeme_len(&t) + 2;
		if (tmp > *w_lexeme)
			*w_lexeme = tmp;
		tmp = num_digits((size_t)t.len);
		if (tmp > *w_len)
			*w_len = tmp;
	}
}
