/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_printf3.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

/* Read a width or precision from the format: literal digits, or '*' which
   consumes the next positional argument, converted with the same strict
   integer rules as %d — bash flags a bad '*' value like a bad %d argument
   (exit 1) while still performing the conversion with the prefix value. */
static long long	pf_scan_num(t_pf *pf, const char *fmt, int *i)
{
	long long	v;

	if (fmt[*i] == '*')
	{
		(*i)++;
		pf->used = true;
		return (pf_num(pf, pf_arg(pf)));
	}
	v = strtoll(fmt + *i, NULL, 10);
	while (ft_isdigit(fmt[*i]))
		(*i)++;
	return (v);
}

/* Parse the flags / width / precision between '%' and the conversion char
   into a t_spec. *i is left on the conversion character (or on the invalid
   byte the caller will reject). A bare '.' means precision zero, per C. */
void	pf_parse_spec(t_pf *pf, const char *fmt, int *i, t_spec *sp)
{
	int	k;

	k = 0;
	*sp = (t_spec){0};
	while (fmt[*i] && ft_strchr("-+ #0", fmt[*i]))
	{
		if (k < 6)
			sp->flags[k++] = fmt[*i];
		(*i)++;
	}
	if (fmt[*i] == '*' || ft_isdigit(fmt[*i]))
	{
		sp->has_width = true;
		sp->width = pf_scan_num(pf, fmt, i);
	}
	if (fmt[*i] == '.')
	{
		(*i)++;
		sp->has_prec = true;
		sp->prec = pf_scan_num(pf, fmt, i);
	}
}

/* Build the canonical snprintf spec "%[flags][width][.prec][ll]conv" from a
   parsed t_spec. Width and precision are re-emitted as digits so values
   sourced from '*' arguments share the literal-digits path; a negative
   width re-enters snprintf as a '-' flag plus positive width, a negative
   precision is dropped entirely — both per C rules. Values are clamped
   (the render buffer grows to match, see printf_wide.c) to keep
   absurd widths from overflowing snprintf's internal int. The "ll" prefix
   makes the vararg stack layout match the long long we actually pass. */
void	pf_build_spec(char *dst, t_spec *sp, char conv)
{
	int	k;

	if (sp->width > PF_WIDTH_MAX)
		sp->width = PF_WIDTH_MAX;
	if (sp->width < -PF_WIDTH_MAX)
		sp->width = -PF_WIDTH_MAX;
	if (sp->prec > PF_WIDTH_MAX)
		sp->prec = PF_WIDTH_MAX;
	dst[0] = '%';
	dst[1] = '\0';
	ft_strlcat(dst, sp->flags, 80);
	k = (int)ft_strlen(dst);
	if (sp->has_width)
		k += snprintf(dst + k, 40, "%lld", sp->width);
	if (sp->has_prec && sp->prec >= 0)
		k += snprintf(dst + k, 40, ".%lld", sp->prec);
	if (ft_strchr("diouxX", conv))
		k += snprintf(dst + k, 4, "ll");
	dst[k] = conv;
	dst[k + 1] = '\0';
}

/* A valid printf -v target: a POSIX identifier, optionally with a
   [subscript] suffix (array element). Rejects digit-initial names and
   an unterminated bracket, matching bash's "not a valid identifier". */
static bool	pf_valid_name(const char *n)
{
	int	i;

	if (!n[0] || (!ft_isalpha((unsigned char)n[0]) && n[0] != '_'))
		return (false);
	i = 1;
	while (n[i] && n[i] != '[')
	{
		if (!ft_isalnum((unsigned char)n[i]) && n[i] != '_')
			return (false);
		i++;
	}
	if (n[i] == '[')
		return (n[ft_strlen(n) - 1] == ']');
	return (true);
}

/* Locate the format word: an optional "--" end-of-options marker is
   consumed (as in `printf -- '-'`); a lone "-" is a format string; any
   other leading dash-word — including bash's -v extension — is rejected as
   an invalid option with status 2, like dash (and like bash for every
   option it does not know). We deliberately do NOT implement -v's variable
   assignment: printf is on the forkless $(...) whitelist precisely because
   it cannot touch shell state, and an in-process `$(printf -v x ...)`
   would leak the assignment into the parent where bash's subshell drops
   it. The spec's expected output for the -v error path (status 2, nothing
   printed) is exactly what this produces. Returns the format's index, or
   -1 on an invalid option. */
int	pf_fmt_index(t_vec argv, char **vname)
{
	char	**av;

	av = (char **)argv.ctx;
	*vname = NULL;
	if (argv.len >= 3 && ft_strcmp(av[1], "-v") == 0 && pf_valid_name(av[2]))
		return (*vname = av[2], 3);
	if (argv.len >= 3 && ft_strcmp(av[1], "-v") == 0)
		return (-1);
	if (argv.len >= 2 && !ft_strncmp(av[1], "--", 3))
		return (2);
	if (argv.len >= 2 && av[1][0] == '-' && av[1][1])
		return (-1);
	return (1);
}
