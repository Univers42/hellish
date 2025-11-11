/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/26 17:54:21 by dlesieur          #+#    #+#             */
/*   Updated: 2025/10/28 16:16:37 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

t_token_type check_key_word(int start, int length, const char *rest, t_token_type type, t_scanner *scan)
{
	if (scan->current - scan->start == start + length && ft_memcmp(scan->start + start, rest, length) == 0)
		return (type);
	return (TOKEN_IDENTIFIER);
}

t_token_type identifier_type(t_scanner *scan)
{
	int					length;
	const t_keyword_entry	*kw;
	int					i;

	i = 0;
	length = (int)(scan->current - scan->start);
	kw = map_keywords();
	while (kw[i].keyword != NULL)
	{
		if (kw[i].length == length &&
			ft_memcmp(scan->start, kw[i].keyword, length) == 0)
			return (kw[i].type);
		i++;
	}
	return (TOKEN_IDENTIFIER);
}

t_token identifier(t_scanner *scan)
{
	char c;
	while (1)
	{
		c = peek(scan);
		if (ft_isalpha(c) || ft_isdigit(c) || c == '_')
		{
			advance(scan);
			continue;
		}
		/* Allow dot as part of identifier only when it's between word chars */
		if (c == '.' && scan->current != scan->start && ft_isalnum(peek_next(scan)))
		{
			advance(scan);
			continue;
		}
		break;
	}
	return (make_token(scan, identifier_type(scan)));
}

t_token number(t_scanner *scan)
{
	while (ft_isdigit(peek(scan)))
		advance(scan);
	if (peek(scan) == '.' && ft_isdigit(peek_next(scan)))
	{
		advance(scan);
		while (ft_isdigit(peek(scan)))
			advance(scan);
	}
	return (make_token(scan, TOKEN_NUMBER));
}

/**
 * @brief handles any quoted string (', "", `)
 * This single function is called when the lexer's dispather
 * seed an opening quote
 * 1. it gests the opening quote character that will be the terminator
 * 2. Determine the correct token type for the entire literal. 
 * 3. Comsume the opening quote (e.g., the '"')
 * 4. Loop to find the end of the string
 * 	handle escaped terminators, but not for single quotes
 * single quotes treat '\' as a literal character. 
 * This checks for things like '"" or '\'
 * @param scan
 * @return one token represeting the whole quoted string
 * 			the parser will later look at this token's lexeme
 * 			to get the actual content.
 * @
 */
t_token string(t_scanner *scan)
{
	char            terminator;
	t_token_type    tok_type;
	t_token         token;

	terminator = peek(scan);
	if (terminator == '"')
		tok_type = TOKEN_DOUBLE_QUOTED_STRING;
	else if (terminator == '\'')
		tok_type = TOKEN_SINGLE_QUOTED_STRING;
	else if (terminator == '`')
		tok_type = TOKEN_BQUOTE;
	else
		return (error_token(scan, "Not a string"));

	/* consume opening quote */
	advance(scan);
	while (peek(scan) != terminator && !scan_is_at_end(scan))
	{
		if (peek(scan) == '\n')
			scan->line++;
		if (terminator != '\'' && peek(scan) == '\\' && peek_next(scan) == terminator)
			advance(scan);
		advance(scan);
	}
	if (scan_is_at_end(scan))
		return (error_token(scan, "Unterminated string"));
	/* consume closing quote */
	advance(scan);

	/* Set token to point to content inside quotes */
	token.type = tok_type;
	token.start = scan->start + 1;  /* Skip opening quote */
	token.length = (int)((scan->current - scan->start) - 2);  /* Exclude both quotes */
	token.line = scan->line;
	return (token);
}
