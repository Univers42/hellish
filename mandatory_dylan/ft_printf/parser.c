/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 13:43:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/07 13:43:16 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	parse_specifier_param(const char *s, int *i, va_list args)
{
	int	ret;

	ret = 0;
	if (s[*i] == '*')
	{
		ret = va_arg(args, int);
		(*i)++;
	}
	else if (s[*i] >= '0' && s[*i] <= '9')
	{
		while (s[*i] >= '0' && s[*i] <= '9')
		{
			ret *= 10;
			ret += s[*i] - '0';
			(*i)++;
		}
	}
	else
	{
		ret = -1;
	}
	return (ret);
}

static int	parse_flags(const char *s, int *i)
{
	int	ret;

	ret = 0;
	while (1)
	{
		if (s[*i] == ' ')
			ret |= FL_SPACE;
		else if (s[*i] == '#')
			ret |= FL_POUND;
		else if (s[*i] == '+')
			ret |= FL_PLUS;
		else if (s[*i] == '-')
			ret |= FL_MINUS;
		else if (s[*i] == '0')
			ret |= FL_ZERO;
		else
			break ;
		(*i)++;
	}
	return (ret);
}

// -1, no presicion specifier
static int	parse_precision(const char *s, int *i, va_list args)
{
	int	ret;

	if (s[*i] != '.')
		return (-1);
	(*i)++;
	ret = parse_specifier_param(s, i, args);
	if (ret == -1)
		ret = 0;
	return (ret);
}

static t_fmt_spec_type	parse_specifier(const char *s, int *i)
{
	int	ret;

	if (s[*i] == 'd' || s[*i] == 'i')
		ret = FMT_INTEGER;
	else if (s[*i] == 'u')
		ret = FMT_UNSIGNED;
	else if (s[*i] == 'x')
		ret = FMT_HEX_LOWER;
	else if (s[*i] == 'X')
		ret = FMT_HEX_UPPER;
	else if (s[*i] == 'p')
		ret = FMT_POINTER;
	else if (s[*i] == 'c')
		ret = FMT_CHAR;
	else if (s[*i] == 's')
		ret = FMT_STRING;
	else if (s[*i] == '%')
		ret = FMT_PERCENT;
	else
		ret = FMT_UNDEFINED;
	(*i)++;
	return (ret);
}

t_fmt_spec	parse(const char *s, int *i, va_list args)
{
	t_fmt_spec	ret;
	int			init_i;

	ret = (t_fmt_spec){0};
	init_i = *i;
	(*i)++;
	if (s[*i - 1] != '%')
		return ((t_fmt_spec){.data = s[*i - 1], .t = FMT_NORMAL_CHAR});
	ret.flags = parse_flags(s, i);
	ret.width = parse_specifier_param(s, i, args);
	ret.precision = parse_precision(s, i, args);
	ret.t = parse_specifier(s, i);
	if (ret.t == FMT_UNDEFINED)
	{
		*i = init_i + 1;
		return ((t_fmt_spec){.data = s[init_i], .t = FMT_NORMAL_CHAR});
	}
	return (ret);
}
