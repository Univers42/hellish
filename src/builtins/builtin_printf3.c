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
   (well above the 4096-byte render buffer, so output is unaffected) to keep
   absurd widths from overflowing snprintf's internal int. The "ll" prefix
   makes the vararg stack layout match the long long we actually pass. */
void	pf_build_spec(char *dst, t_spec *sp, char conv)
{
	int	k;

	if (sp->width > 30000)
		sp->width = 30000;
	if (sp->width < -30000)
		sp->width = -30000;
	if (sp->prec > 30000)
		sp->prec = 30000;
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
