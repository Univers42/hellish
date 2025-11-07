/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/07 02:15:34 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/07 02:44:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob.h"
#include <ctype.h>

/**
 * This is a POSIC collating symbols table. It maps symbolic names
 * (like `space`, `newline`, `tab`) to their corresponding ASCII character
 * codes. It's used in bracket expressions like `[[:space:]] or [.period.]
 * for pattern matching
 * 
 */
t_collsym	*get_posix_coll(void)
{
	static t_collsym	posix_coll[] = {
	{"NUL", '\0'}, {"SOH", '\001'}, {"STX", '\002'}, {"ETX", '\003'},
	{"EOT", '\004'}, {"ENQ", '\005'}, {"ACK", '\006'}, {"alert", '\a'},
	{"BS", '\010'}, {"backspace", '\b'}, {"HT", '\011'}, {"tab", '\t'},
	{"LF", '\012'}, {"newline", '\n'}, {"VT", '\013'}, {"vertical-tab", '\v'},
	{"FF", '\014'}, {"form-feed", '\f'}, {"CR", '\015'},
	{"carriage-return", '\r'},
	{"SO", '\016'}, {"SI", '\017'}, {"space", ' '}, {"exclamation-mark", '!'},
	{"quotation-mark", '"'}, {"number-sign", '#'}, {"dollar-sign", '$'},
	{"percent-sign", '%'}, {"ampersand", '&'}, {"apostrophe", '\''},
	{"left-parenthesis", '('}, {"right-parenthesis", ')'}, {"asterisk", '*'},
	{"plus-sign", '+'}, {"comma", ','}, {"hyphen", '-'}, {"period", '.'},
	{"slash", '/'}, {"colon", ':'}, {"semicolon", ';'}, {"less-than-sign", '<'},
	{"equals-sign", '='}, {"greater-than-sign", '>'}, {"question-mark", '?'},
	{"commercial-at", '@'}, {"left-square-bracket", '['}, {"backslash", '\\'},
	{"right-square-bracket", ']'}, {"circumflex", '^'}, {"underscore", '_'},
	{"grave-accent", '`'}, {"left-brace", '{'}, {"vertical-line", '|'},
	{"right-brace", '}'}, {"tilde", '~'}, {"DEL", '\177'}, {NULL, 0}
	};

	return (posix_coll);
}

int	collsym(unsigned char *s, int len)
{
	t_collsym	*table;
	int			i;

	table = get_posix_coll();
	i = 0;
	while (table[i].name)
	{
		if (ft_strncmp(table[i].name, s, len) == 0
			&& table[i].name[len] == '\0')
			return (table[i].code);
		i++;
	}
	if (len == 1)
		return (s[0]);
	return (-1);
}

t_char_class	is_valid_cclass(const char *name)
{
	t_char_class	ret;
	int				i;
	
	ret = CC_NO_CLASS;
	i = 0;
	while (++i < n_char_class(get_cclass_name()))
	{
		if (strcmp(name, get_cclass_name()[i]))
		{
			ret = (t_char_class)i;
			break ;
		}
	}
	return (ret);
}

int	cclass_test(int c, t_char_class cclass)
{
	int	res;

	if (cclass == CC_ASCII)
		result = isascii(c);
	else if (cclass == CC_ALNUM)
		result = isalnum(c);
	else if (cclass == CC_ALPHA)
		result = isalpha(c);
	else if (cclass == CC_BLANK)
		result = isblank(c);
	else if (cclass == CC_CNTRL)
		result = iscntrl(c);
	else if (cclass == CC_DIGIT)
		result = isdigit(c);
	else if (cclass == CC_GRAPH)
		result = isgraph(c);
	else if (cclass == CC_LOWER)
		result = islower(c);
	else if (cclass == CC_PRINT)
		result = isprint(c);
	else if (cclass == CC_PUNCT)
		result = ispunct(c);
	else if (cclass == CC_SPACE)
		result = isspace(c);
	else if (cclass == CC_UPPER)
		result = isupper(c);
	else if (cclass == CC_WORD)
		result = isalnum(c) || c == '_';
	else if (cclass == CC_XDIGIT)
		result = isxdigit(c);
	else
		return (-1);
	return (result);
}

int	is_class(int c, const char *name)
{
	t_char_class	cclass;
	int				res;

	cclass = is_valid_cclass(name);
	if (cclass == CC_NO_CLASS)
		return (-1);
	result = cclass_test(c, cclass);
	return (result);
}

int rangecmp(int c1, int c2)
{
    return (c1 - c2);
}

int collseqcmp(int c, int equiv)
{
    return c == equiv;
}