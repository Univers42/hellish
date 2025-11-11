/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers1.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/26 17:51:17 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/02 17:52:32 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

char advance(t_scanner *scan)
{
	scan->current++;
	return (scan->current[-1]);
}

char peek(t_scanner *scan)
{
	return *(scan->current);
}

bool scan_is_at_end(t_scanner *scan)
{
	return (*scan->current == '\0');
}

char peek_next(t_scanner *scan)
{
	if (scan_is_at_end(scan))
		return ('\0');
	return (scan->current[1]);
}

bool match(t_scanner *scan, char expected)
{
	if (scan_is_at_end(scan))
		return (false);
	if (*(scan->current) != expected)
		return (false);
	scan->current++;
	return (true);
}

t_token make_token(t_scanner *scan, t_token_type type)
{
	t_token token;

	token.type = type;
	token.start = scan->start;
	token.length = (int)(scan->current - scan->start);
	token.line = scan->line;
	scan->last_token = type; // track last emitted token
	return (token);
}

t_token error_token(t_scanner *scan, const char *mesg)
{
	t_token token;

	token.type = TOKEN_ERR;
	token.start = mesg;
	token.length = (int)strlen(mesg);
	token.line = scan->line;
	return (token);
}

void skip_unused_data(t_scanner *scan)
{
	char c;

	while (ST_SCANNING)
	{
		c = peek(scan);
		if (ft_isspace(c))
			advance(scan);
		if (assert_char(c, '\n'))
		{
			scan->line++;
			advance(scan);
		}
		if (assert_char(c, '/'))
		{
			if (peek_next(scan) == '/')
			{
				while (peek(scan) != '\n' && !scan_is_at_end(scan))
					advance(scan);
			}
			else
				return;
		}
		else
			break;
	}
	return;
}
