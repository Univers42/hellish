/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   printf_bytes.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 19:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/06 19:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

/* %b, padded by hand rather than through snprintf("%s").
**
** A %b argument is the one conversion whose bytes come from escape
** decoding, so it can legitimately contain a NUL: `printf '%b' 'a\0b'` is
** three bytes in bash, and `printf '%b' '\0'` is the idiom for emitting
** one.  Both used to stop at the NUL here, because the decoded string was
** handed to vec_push_str and to snprintf, and each of those measures with
** strlen.  Width and precision are applied by byte count, which is what
** bash's printstr does for %b as well. */

static void	push_bytes(t_string *out, const char *s, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
		vec_push_char(out, s[i++]);
}

static void	push_spaces(t_string *out, long long n)
{
	while (n-- > 0)
		vec_push_char(out, ' ');
}

/* \c inside the argument aborts the whole printf -- AFTER this field is
   laid out: bash pads and truncates what came before the \c as if it were
   the whole argument (`printf '%-6b|' 'ab\c'` is "ab" and four spaces,
   and no bar), and only then stops. */
void	pf_emit_b_padded(t_pf *pf, t_spec *sp, const char *arg)
{
	t_string	raw;
	size_t		n;
	long long	pad;

	vec_init(&raw);
	raw.elem_size = 1;
	pf_emit_b(pf, &raw, arg);
	n = raw.len;
	if (sp->has_prec && sp->prec >= 0 && (long long)n > sp->prec)
		n = (size_t)sp->prec;
	pad = 0;
	if (sp->has_width)
		pad = sp->width - (long long)n;
	if (!ft_strchr(sp->flags, '-'))
		push_spaces(pf->out, pad);
	push_bytes(pf->out, (const char *)raw.ctx, n);
	if (ft_strchr(sp->flags, '-'))
		push_spaces(pf->out, pad);
	xfree(raw.ctx);
}
