/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_unum.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/20 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"
#include <errno.h>
#include <stdlib.h>

/* The unsigned half of printf's numeric argument parsing.

   %u/%o/%x/%X went through pf_num like everything else, which parses with
   strtoll -- so any value above LLONG_MAX saturated there, long before the
   conversion ran:

       printf "%u\n" 18446744073709551615
         bash     18446744073709551615
         hellish   9223372036854775807

   The printed spec was never the problem (pf_build_spec already injects
   "ll", so snprintf sees %llu); the value handed to it had already been
   clamped. strtoull is the whole fix.

   Negatives are deliberately NOT rejected: strtoull negates in unsigned
   arithmetic, so `printf %u -1` gives 18446744073709551615, exactly what
   bash prints. Genuine overflow still sets ERANGE and is reported, which
   matches bash exiting 1 while still emitting the clamped prefix. A
   leading quote yields the next byte's code point, as for the signed
   conversions; a missing argument is silently zero. */
t_ull	pf_unum(t_pf *pf, const char *arg)
{
	char	*end;
	t_ull	v;

	if (!arg)
		return (0);
	if (arg[0] == '\'' || arg[0] == '"')
		return ((unsigned char)arg[1]);
	errno = 0;
	v = strtoull(arg, &end, 0);
	if (end == arg || *end != '\0' || errno == ERANGE)
		pf_err_num(pf, arg);
	return (v);
}

/* The floating-point conversions (%e %f %g %a and friends). Split out of
   pf_conv_str only so that function keeps room for the signed/unsigned
   split above it. bash prints the converted prefix of a malformed number
   and still exits 1, which is why the error is reported but the value is
   used anyway. */
void	pf_conv_float(t_pf *pf, char *fmt, const char *arg, t_pfbuf *b)
{
	char	*end;
	double	d;

	d = 0.0;
	errno = 0;
	end = (char *)arg;
	if (arg)
		d = strtod(arg, &end);
	if (arg && (end == arg || *end != '\0' || errno == ERANGE))
		pf_err_num(pf, arg);
	snprintf(b->p, b->cap, fmt, d);
}
