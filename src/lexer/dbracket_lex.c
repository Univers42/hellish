/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dbracket_lex.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "helpers.h"

/* Is `str` exactly the 2-char token `tok` ([[ or ]]) at a word boundary? */
static int	is_dbracket_tok(const char *str, const char *tok)
{
	if (ft_strncmp(str, tok, 2) != 0)
		return (0);
	if (str[2] == '\0' || is_space(str[2]) || str[2] == '\n')
		return (1);
	return (0);
}

/* Toggle conditional mode on `[[` / `]]`. While set, the tokenizer emits
   &&, ||, ( and ) as WORD tokens so the parser does not split the command. */
void	dbracket_toggle(const char *str, int *in_db)
{
	if (!*in_db && is_dbracket_tok(str, "[["))
		*in_db = 1;
	else if (*in_db && is_dbracket_tok(str, "]]"))
		*in_db = 0;
}

/* Inside [[ ]], emit &&/||/(/) as a literal WORD; return 1 if emitted. */
int	emit_dbracket_word(char **str, t_deque_tok *ret)
{
	t_token	tmp;
	int		len;

	len = 0;
	if (ft_strncmp(*str, "&&", 2) == 0 || ft_strncmp(*str, "||", 2) == 0)
		len = 2;
	else if (**str == '(' || **str == ')')
		len = 1;
	if (len == 0)
		return (0);
	tmp = create_token(*str, len, TT_WORD);
	deque_push_end(&ret->deqtok, &tmp);
	*str += len;
	return (1);
}
