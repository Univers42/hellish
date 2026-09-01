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
	if (kw_eq(*str, "&&", 2) || kw_eq(*str, "||", 2))
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

/* Inside [[ ]], a newline is whitespace exactly where bash's conditional
   grammar tolerates one: after the [[ itself, after && / || / ( / !, and
   before ]] / && / || / ). Anywhere else it stays a real token, so
   `[[ $x\n== y ]]` still fails like bash. bash-completion 2.16 writes
   `[[ a != @(...) &&\n -f b ]]` in its compat-dir loop, and emitting that
   newline cut the conditional in half: "[[: missing ]]" once per drop-in
   file at every Debian 13 login, then the orphaned tail ran as commands
   ("-f: command not found") -- issue #105's second wave. `after` points
   just past the newline. */
bool	db_newline_skippable(t_deque_tok *ret, const char *after)
{
	t_ltoken	*last;
	char		*s;

	while (*after == ' ' || *after == '\t')
		after++;
	if ((after[0] == ']' && after[1] == ']')
		|| (after[0] == '&' && after[1] == '&')
		|| (after[0] == '|' && after[1] == '|') || after[0] == ')')
		return (true);
	if (ret->deqtok.len == 0)
		return (false);
	last = (t_ltoken *)deque_idx(&ret->deqtok, ret->deqtok.len - 1);
	s = ret->base + last->off;
	if (last->len == 2 && ((s[0] == '[' && s[1] == '[')
			|| (s[0] == '&' && s[1] == '&') || (s[0] == '|' && s[1] == '|')))
		return (true);
	if (last->len == 1 && (s[0] == '(' || s[0] == '!'))
		return (true);
	return (false);
}
