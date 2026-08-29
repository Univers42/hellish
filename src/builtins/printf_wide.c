/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_wide.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 14:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 14:20:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"
#include "builtins_private.h"

/* Wide conversions: `printf "%10000000s" c` (issue #73).
**
** Output used to stop dead at 4095 bytes for ANY width, because pf_conv
** rendered into a fixed char[4096]. There was a second cap above it --
** pf_build_spec clamped width to 30000, with a comment claiming that was
** "well above the 4096-byte render buffer, so output is unaffected", which
** is exactly backwards: 30000 > 4096, so the clamp hid nothing and the
** buffer truncated everything. Two caps, both silent, and bash produces the
** full field.
**
** The size comes from the SPEC, not from a trial run. snprintf would happily
** report the length it needed, but asking it means formatting twice -- and
** pf_num/pf_unum report conversion errors as a side effect, so a second pass
** would print every "invalid number" diagnostic twice. Width and precision
** are known before any conversion runs, so one measurement and one format.
**
** The stack buffer stays the fast path: anything that fits in 4096 bytes --
** which is every conversion anyone actually writes -- allocates nothing. */

/* Bytes this conversion can need: the field width or the precision,
   whichever is larger, plus room for sign, prefix and the digits themselves.
   Negative widths are left-justification, so they count by magnitude. */
size_t	pf_render_size(t_spec *sp)
{
	long long	n;
	long long	w;

	n = 0;
	w = sp->width;
	if (w < 0)
		w = 0 - w;
	if (sp->has_width && w > n)
		n = w;
	if (sp->has_prec && sp->prec > n)
		n = sp->prec;
	if (n > PF_WIDTH_MAX)
		n = PF_WIDTH_MAX;
	return ((size_t)n + PF_STACK_BUF);
}

/* Render into `stack` when it fits, onto the heap when it does not, and push
   the result. Falling back to the stack buffer on a failed allocation keeps
   a 2GB width from taking the shell down -- the output is truncated, which
   is what happened before this existed anyway. */
void	pf_emit_sized(t_pf *pf, t_spec *sp, char *fmt, const char *arg)
{
	char		stack[PF_STACK_BUF];
	t_pfbuf		b;

	b.cap = pf_render_size(sp);
	b.p = stack;
	if (b.cap > PF_STACK_BUF)
		b.p = xmalloc(b.cap);
	if (!b.p)
	{
		b.p = stack;
		b.cap = PF_STACK_BUF;
	}
	b.p[0] = '\0';
	pf_conv_str(pf, fmt, arg, &b);
	vec_push_str(pf->out, b.p);
	if (b.p != stack)
		xfree(b.p);
}
