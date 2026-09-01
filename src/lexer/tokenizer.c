/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helper1.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:04:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:04:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "ft_glob.h"
#include "helpers.h"

bool	is_word_boundary(const char *s);
int		extglob_ahead(const char *at);

/* Decide whether the current position starts a quoted/dollar/word lexeme
   that needs parse_lexeme, or a bare operator. The OR with !is_word_boundary
   is the catch-all: any run of non-special chars is a plain word.
     zsh's `=(cmd)` has to be pulled out first: `=` is an ordinary word
   character, so the catch-all would swallow it and leave the `(` to open a
   subshell -- which is what `cat =(echo hi)` did, a syntax error. Gated on
   the dialect; in bash a leading `=` really is just a word character.
     extglob is the mirror case: `!(a)` and `*(a)` START with characters the
   catch-all would hand to the operator path, so the group has to be spotted
   here as well as inside parse_lexeme's loop.
     Inside [[ ]] the extglob cell is armed for the span of the lexeme:
   bash recognises extglob groups in conditional operands whether or not
   `shopt -s extglob` is set (4.1+), so `[[ $x == a@(b|z)c ]]` must lex the
   group as one word -- unarmed, the group's `|` became a PIPE and the
   conditional shattered (issue #105; the matcher's half of the same rule
   is db_pattern_match). */
static char	*try_parse_lexeme(char **str, t_deque_tok *ret, int in_db)
{
	char	*prompt;
	int		saved;

	if (glob_zsh() && (*str)[0] == '=' && (*str)[1] == '(')
		return (0);
	saved = *glob_extglob_cell();
	if (in_db)
		*glob_extglob_cell() = 1;
	prompt = 0;
	if (**str == '\'' || **str == '"' || **str == '$'
		|| extglob_ahead(*str) || !(is_word_boundary(*str)))
		prompt = parse_lexeme(ret, str);
	*glob_extglob_cell() = saved;
	return (prompt);
}

/* Emit a single TT_NEWLINE token and advance past the '\n'. Newlines are
   kept as real tokens (not skipped like spaces) because the grammar uses
   them as statement separators inside compound commands. */
static void	emit_newline(char **str, t_deque_tok *ret)
{
	t_token	tmp;

	tmp = create_token(*str, 1, TT_NEWLINE);
	push_ltok(ret, tmp);
	(*str)++;
}

/* Skip inter-token noise: shell comments and backslash-newline line
   continuations. The continuation must be removed here, not tokenised, else a
   `\<newline>` becomes a spurious empty/newline argument (notably in sourced
   function bodies, which the REPL's line-joining never reaches). */
bool	skip_noise(char **str)
{
	if (**str == '#')
	{
		while (**str && **str != '\n')
			(*str)++;
		return (true);
	}
	if ((*str)[0] == '\\' && (*str)[1] == '\n')
		return (*str += 2, true);
	return (false);
}

/* One tokenizing step: toggle [[ ]] mode, parse a lexeme, or emit an
   operator. Inside [[ ]], &&/||/(/) are emitted as WORD tokens so the parser
   does not split the conditional. Returns a continuation prompt if a lexeme
   is unterminated, else NULL. */
char	*tokenize_step(char **str, t_deque_tok *ret, int *in_db)
{
	char	*prompt;

	if (db_regex_word(str, ret, in_db))
		return (NULL);
	if (**str == '[' || (*in_db && **str == ']'))
		dbracket_toggle(*str, in_db);
	prompt = try_parse_lexeme(str, ret, *in_db);
	if (prompt)
		return (prompt);
	db_track_regex(ret, in_db);
	if (**str == '\n' && *in_db && db_newline_skippable(ret, *str + 1))
		(*str)++;
	else if (**str == '\n')
		emit_newline(str, ret);
	else if (is_space(**str))
		(*str)++;
	else if (*in_db && emit_dbracket_word(str, ret))
		return (NULL);
	else if (**str)
		parse_op(ret, str);
	return (NULL);
}

/* Top-level tokenizer. Walk the raw input string until exhausted, producing
   a token deque that ends with TT_END. Returns NULL on success or a
   continuation-prompt string when input is incomplete (unterminated quote,
   open subshell, line continuation). The caller re-prompts the user with
   that string and appends more text before calling back. The deque is cleared
   at the start so the same t_deque_tok can be reused across REPL turns
   without leaking the previous command's tokens. */
char	*tokenizer(char *str, t_deque_tok *ret)
{
	char	*prompt;
	t_token	tmp;
	int		in_db;

	prompt = 0;
	in_db = 0;
	ret->base = str;
	deque_clear(&ret->deqtok, NULL);
	while (str && *str)
	{
		if (skip_noise(&str))
			continue ;
		prompt = tokenize_step(&str, ret, &in_db);
		if (prompt)
			break ;
	}
	tmp = create_token(0, 0, TT_END);
	push_ltok(ret, tmp);
	return (prompt);
}
