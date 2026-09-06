/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dbracket_lex2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"
#include "helpers.h"
#include "case_match.h"

/* Regex-word mode for [[ x =~ ERE ]]. In conditional mode the lexer
   normally emits ( and ) as grouping words, which would shred a regex
   like ^(.+)=([0-9]+)$ into five tokens. bash's rule: the word after
   =~ is ONE token, terminated only by whitespace. in_db carries the
   state: 1 = inside [[ ]], 2 = the next word is a regex. */

/* After a token lands while in conditional mode, arm regex mode when
   that token is the =~ operator. */
void	db_track_regex(t_deque_tok *ret, int *in_db)
{
	t_ltoken	*last;
	char		*s;

	if (*in_db != 1 || ret->deqtok.len == 0)
		return ;
	last = (t_ltoken *)deque_idx(&ret->deqtok, ret->deqtok.len - 1);
	s = ret->base + last->off;
	if (last->tt == TT_WORD && last->len == 2
		&& s[0] == '=' && s[1] == '~')
		*in_db = 2;
}

/* Consume the whole regex word (everything up to UNQUOTED whitespace) and
** emit it as a single TT_WORD; drop back to plain conditional mode.
** Returns 1 when a regex token was emitted.
**
** "unquoted" is the load-bearing word and it was missing: the scan stopped
** at the first space wherever it sat, so a quoted right-hand side with a
** space in it came apart mid-quote --
**
**     [[ $x =~ "declare -a" ]]
**
** left `"declare` as the token and `-a"` behind it, and the shell answered
** `unexpected EOF while looking for matching '"'` for a line of perfectly
** ordinary bash. That line is in Ubuntu's own /etc/profile.d/vte-2.91.sh,
** so every GNOME desktop login printed a syntax error -- issue #51's
** complaint, arriving from a file #51 never looked at. A backslash-escaped
** space belongs to the word too.
*/
int	db_regex_word(char **str, t_deque_tok *ret, int *in_db)
{
	t_token	tmp;
	char	*start;

	if (*in_db != 2 || !**str || is_space(**str) || **str == '\n')
		return (0);
	start = *str;
	while (**str && !is_space(**str) && **str != '\n')
	{
		if (**str == '\\' && (*str)[1])
			(*str) += 2;
		else if (**str == '\'')
			advance_squoted(str);
		else if (**str == '"')
			advance_dquoted(str);
		else
			(*str)++;
	}
	tmp = create_token(start, (int)(*str - start), TT_WORD);
	push_ltok(ret, tmp);
	*in_db = 1;
	return (1);
}

/* Armed for the span of one lexeme inside [[ ]] (tokenizer.c) so a zsh
   alternation may OPEN the word: `[[ $TERM == (xterm*|screen*) ]]`.
   Everywhere else a `(` at the front of a word is a subshell or a
   function header, and zsh_alt_ahead keeps refusing it
   (case_match_ext3.c).  A cell rather than a parameter for the reason
   the extglob cell is one: the word lexer has no [[ state of its own,
   and every path into zsh_alt_ahead would otherwise grow an argument
   it does not understand. */
int	*db_front_cell(void)
{
	static int	on;

	return (&on);
}

/* The alternation a [[ operand may open with, or 0.  Only while the cell
   is armed, and only a group with no blank inside: `( $x == a || $y )` is
   [['s own grouping, and a pattern never holds an unquoted space.  This
   was the "[[: missing `]]'" every `[[ $x == (a|b) ]]` in a zsh rc got:
   the `(` became its own word, the `|` a pipe, and the conditional
   shattered into commands named after the alternatives. */
int	db_front_group(const char *at)
{
	int	n;
	int	i;

	if (!*db_front_cell())
		return (0);
	n = xg_alt_group(at);
	i = 0;
	while (i < n)
	{
		if (at[i] == ' ' || at[i] == '\t' || at[i] == '\n')
			return (0);
		i++;
	}
	return (n);
}
