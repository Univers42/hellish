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

/* Consume the whole regex word (everything up to unquoted whitespace)
   and emit it as a single TT_WORD; drop back to plain conditional mode.
   Returns 1 when a regex token was emitted. */
int	db_regex_word(char **str, t_deque_tok *ret, int *in_db)
{
	t_token	tmp;
	char	*start;

	if (*in_db != 2 || !**str || is_space(**str) || **str == '\n')
		return (0);
	start = *str;
	while (**str && !is_space(**str) && **str != '\n')
		(*str)++;
	tmp = create_token(start, (int)(*str - start), TT_WORD);
	push_ltok(ret, tmp);
	*in_db = 1;
	return (1);
}
