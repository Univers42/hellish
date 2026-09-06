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
** The size comes from the SPEC and the ARGUMENT, not from a trial run.
** snprintf would happily report the length it needed, but asking it means
** formatting twice -- and pf_num/pf_unum report conversion errors as a side
** effect, so a second pass would print every "invalid number" diagnostic
** twice. Width and precision are known before any conversion runs, and so
** is the argument's length, so one measurement and one format.
**
** The argument matters as much as the width: `printf '%s\n' "$block"` with
** a 5 KB block -- a docker-compose services section captured by $( ) and
** fed back through printf, as Inception's compliance suite does -- stopped
** at 4096 bytes and lost 31 lines, with no width in the spec at all.
**
** The stack buffer stays the fast path: anything that fits in 4096 bytes --
** which is every conversion anyone actually writes -- allocates nothing. */

/* Bytes this conversion can need: the field width, the precision or the
   argument's own length, whichever is largest, plus room for sign, prefix
   and the digits themselves. Negative widths are left-justification, so
   they count by magnitude. */
size_t	pf_render_size(t_spec *sp, size_t arglen)
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
	if (arglen < (size_t)PF_WIDTH_MAX && (long long)arglen > n)
		n = (long long)arglen;
	if (n > PF_WIDTH_MAX)
		n = PF_WIDTH_MAX;
	return ((size_t)n + PF_STACK_BUF);
}

/* Point `b` at somewhere big enough for this spec: the caller's stack buffer
   when it fits, the heap when it does not. Falling back to the stack on a
   failed allocation keeps a 2GB width from taking the shell down -- the
   output is truncated, which is what happened before any of this existed.
**
** Every conversion goes through this. It has to: %c, %b and the float
** conversions each used to carry their OWN char[4096], so fixing only the
** snprintf-delegated path left `printf "%50000c"` still stopping at 4095.
** Four places owning one number is the actual defect; this is the one
** number. */
void	pf_buf_open(t_spec *sp, t_pfbuf *b, char *stack, size_t arglen)
{
	b->cap = pf_render_size(sp, arglen);
	b->p = stack;
	if (b->cap > PF_STACK_BUF)
		b->p = xmalloc(b->cap);
	if (!b->p)
	{
		b->p = stack;
		b->cap = PF_STACK_BUF;
	}
	b->p[0] = '\0';
}

void	pf_buf_close(t_pfbuf *b, char *stack)
{
	if (b->p != stack)
		xfree(b->p);
}

/* Render one conversion and push it. */
void	pf_emit_sized(t_pf *pf, t_spec *sp, char *fmt, const char *arg)
{
	char		stack[PF_STACK_BUF];
	t_pfbuf		b;

	if (arg)
		pf_buf_open(sp, &b, stack, ft_strlen(arg));
	else
		pf_buf_open(sp, &b, stack, 0);
	pf_conv_str(pf, fmt, arg, &b);
	vec_push_str(pf->out, b.p);
	pf_buf_close(&b, stack);
}

/* %b with a field width. pf_emit_b expands backslash escapes straight into
   the output, so the spec never reached it and `printf "%10b" ab` printed
   "ab" where bash prints "        ab". Expand into a scratch string first,
   then pad it exactly like every other conversion.
**
** \c inside the argument aborts the whole printf, so a stopped expansion is
   emitted as-is and never padded -- padding output that was cut short would
   invent characters the format asked to stop before. */
void	pf_emit_b_padded(t_pf *pf, t_spec *sp, const char *arg)
{
	char		stack[PF_STACK_BUF];
	char		fmt[80];
	t_string	raw;
	t_pfbuf		b;

	vec_init(&raw);
	raw.elem_size = 1;
	pf_emit_b(&raw, arg, &pf->stop);
	vec_push_char(&raw, '\0');
	if (!sp->has_width || pf->stop)
		return (vec_push_str(pf->out, (char *)raw.ctx), (void)xfree(raw.ctx));
	pf_build_spec(fmt, sp, 's');
	pf_buf_open(sp, &b, stack, raw.len);
	snprintf(b.p, b.cap, fmt, (char *)raw.ctx);
	vec_push_str(pf->out, b.p);
	pf_buf_close(&b, stack);
	xfree(raw.ctx);
}
