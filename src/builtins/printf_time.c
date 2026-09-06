/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_time.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"
#include "env.h"
#include <time.h>

/* %(fmt)T -- bash's strftime conversion, issue #122.  The argument is
   seconds since the epoch; -1 is now, -2 the moment the shell started, and
   no argument at all behaves like -1.  Everything after the format is the
   %s machinery: width, precision and flags apply to the rendered string. */

#define PF_TIMEFMT_MAX 256
#define PF_TIMEBUF_MAX 256

/* Copy the strftime format between the parentheses at fmt[*i] == '(' into
   buf, parentheses nesting the way bash scans them, and leave *i on the
   byte after the closing one.  False when the format never closes, does
   not fit, or is not followed by the T conversion byte. */
static bool	pf_time_fmt(const char *fmt, int *i, char *buf)
{
	int	depth;
	int	n;

	depth = 1;
	n = 0;
	(*i)++;
	while (fmt[*i] && depth > 0 && n < PF_TIMEFMT_MAX - 1)
	{
		if (fmt[*i] == '(')
			depth++;
		else if (fmt[*i] == ')')
			depth--;
		if (depth > 0)
			buf[n++] = fmt[(*i)++];
	}
	buf[n] = '\0';
	if (fmt[*i] != ')' || fmt[*i + 1] != 'T')
		return (false);
	*i += 2;
	return (true);
}

/* The seconds the argument names.  bash takes an argument only when one
   is left (a bare `printf '%(%Y)T'` prints the current year), and parses
   it with the same strict integer rules as %d, so junk is reported and
   reads as 0 -- the epoch -- exactly as there. */
static time_t	pf_time_secs(t_pf *pf)
{
	const char	*arg;
	long long	v;

	v = -1;
	arg = pf_arg(pf);
	if (arg)
		v = pf_num(pf, arg);
	if (v == -1)
		return (time(NULL));
	if (v == -2)
		return ((time_t)pf->state->start_sec);
	return ((time_t)v);
}

/* libc's environ is the snapshot exec handed us; an `export TZ=UTC` in the
   script or a `TZ=UTC printf ...` prefix lives only in the shell's own env.
   Push the shell's TZ into libc right before localtime reads it, which is
   what bash's sv_tz("TZ") does at the same spot. */
static void	pf_time_tz(t_pf *pf)
{
	char	*tz;

	tz = env_expand(pf->state, "TZ");
	if (tz)
		setenv("TZ", tz, 1);
	else
		unsetenv("TZ");
	tzset();
}

/* Render secs through strftime into out.  An empty format means "%X", the
   locale's time, as in bash; a time localtime cannot represent falls back
   to the epoch rather than printing garbage. */
static void	pf_time_render(const char *tfmt, time_t secs, char *out)
{
	struct tm	*tm;

	tm = localtime(&secs);
	if (!tm)
	{
		secs = 0;
		tm = localtime(&secs);
	}
	out[0] = '\0';
	if (!tfmt[0])
		tfmt = "%X";
	if (tm && !strftime(out, PF_TIMEBUF_MAX, tfmt, tm))
		out[0] = '\0';
}

/* Entry: *i on the '(' after the spec.  On a malformed spec bash warns,
   prints the '%' as a literal and resumes scanning right after it -- the
   caller does that on a false return, so the flags and the parenthesis
   come out as plain text, byte for byte like bash. */
bool	pf_conv_time(t_pf *pf, t_spec *sp, const char *fmt, int *i)
{
	char	tfmt[PF_TIMEFMT_MAX];
	char	out[PF_TIMEBUF_MAX];
	char	spec[80];
	time_t	secs;

	if (!pf_time_fmt(fmt, i, tfmt))
	{
		ft_eprintf("%s: printf: warning: `%c': invalid time format "
			"specification\n", pf->ctx, fmt[*i]);
		return (false);
	}
	pf->used = true;
	secs = pf_time_secs(pf);
	pf_time_tz(pf);
	pf_time_render(tfmt, secs, out);
	pf_build_spec(spec, sp, 's');
	pf_emit_sized(pf, sp, spec, out);
	return (true);
}
