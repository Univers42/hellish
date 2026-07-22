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

/* A `[[`/`]]` token ends at whitespace, end-of-string, or any shell
   metacharacter that terminates a word (`; & | ( ) < >`). Without `;` & co
   here, `]]` before `;` (e.g. `[[ x ]];`) was not recognised and conditional
   mode leaked into the rest of the input, mangling later `()`/braces. */
static int	is_db_boundary(char c)
{
	if (c == '\0' || c == '\n' || is_space(c))
		return (1);
	if (c == ';' || c == '&' || c == '|')
		return (1);
	if (c == '(' || c == ')' || c == '<' || c == '>')
		return (1);
	return (0);
}

/* Is `str` exactly the 2-char token `tok` ([[ or ]]) at a word boundary? */
static int	is_dbracket_tok(const char *str, const char *tok)
{
	if (ft_strncmp(str, tok, 2) != 0)
		return (0);
	return (is_db_boundary(str[2]));
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

/* Inside [[ ]], emit &&/||/(/) — and the string-comparison operators
   < and > — as literal WORDs; return 1 if emitted. Without the < > case
   they tokenised as REDIRECTS, so `[[ a < b ]]` silently opened file b
   for stdin instead of comparing (the test evaluator understood the
   operators all along; they just never reached it). */
int	emit_dbracket_word(char **str, t_deque_tok *ret)
{
	t_token	tmp;
	int		len;

	len = 0;
	if (ft_strncmp(*str, "&&", 2) == 0 || ft_strncmp(*str, "||", 2) == 0)
		len = 2;
	else if (**str == '(' || **str == ')')
		len = 1;
	else if (**str == '<' || **str == '>')
		len = 1;
	if (len == 0)
		return (0);
	tmp = create_token(*str, len, TT_WORD);
	push_ltok(ret, tmp);
	*str += len;
	return (1);
}
