/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_printf.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

/* Consume the next positional argument from the format call's argv. Returns
   NULL when all arguments are exhausted — conversions that receive NULL
   treat it as an empty string or zero, matching POSIX printf behaviour. */
const char	*pf_arg(t_pf *pf)
{
	if (pf->argi < pf->argc)
		return (pf->av[pf->argi++]);
	return (NULL);
}

/* Flag a bad numeric argument the way bash does: complain on stderr, keep
   converting with whatever value the prefix yielded, and remember to exit 1
   once the whole format has been processed. */
void	pf_err_num(t_pf *pf, const char *arg)
{
	pf->err = 1;
	ft_eprintf("%s: printf: %s: invalid number\n", pf->ctx, arg);
}

/* Render one snprintf-delegated conversion into `buf` (4096 bytes). The
   conversion char is the spec's last byte. Signed integers go through
   pf_num, UNSIGNED ones through pf_unum -- routing %u/%o/%x/%X through the
   signed parser clamped every value above LLONG_MAX (issue #28). Floats
   get the same trailing-junk treatment via strtod in pf_conv_float — bash
   prints the converted prefix but still exits 1. */
int	pf_conv_str(t_pf *pf, char *fmt, const char *arg, t_pfbuf *b)
{
	char	conv;

	conv = fmt[ft_strlen(fmt) - 1];
	if (conv == 's')
	{
		if (!arg)
			arg = "";
		return (snprintf(b->p, b->cap, fmt, arg));
	}
	if (ft_strchr("ouxX", conv))
		return (snprintf(b->p, b->cap, fmt, pf_unum(pf, arg)));
	if (ft_strchr("di", conv))
		return (snprintf(b->p, b->cap, fmt, pf_num(pf, arg)));
	pf_conv_float(pf, fmt, arg, b);
	return ((int)ft_strlen(b->p));
}

/* %c prints the first byte of its argument — bash emits a real NUL byte
   when the argument is empty or missing — honouring width and the '-'
   flag but ignoring precision. The rendered bytes are pushed by count,
   not by strlen, precisely because of that possible embedded NUL. */
static void	pf_conv_char(t_pf *pf, t_spec *sp, const char *arg)
{
	char	stack[PF_STACK_BUF];
	char	fmt[80];
	t_pfbuf	b;
	int		n;
	int		i;

	i = 0;
	if (arg)
		i = (unsigned char)arg[0];
	sp->has_prec = false;
	pf_build_spec(fmt, sp, 'c');
	pf_buf_open(sp, &b, stack);
	n = snprintf(b.p, b.cap, fmt, i);
	if (n >= (int)b.cap)
		n = (int)b.cap - 1;
	i = 0;
	while (i < n)
		vec_push_char(pf->out, b.p[i++]);
	pf_buf_close(&b, stack);
}

/* Handle one conversion: %%, %c, %b (backslash-escape string), %q (shell
   quoting), and the full snprintf-delegated set. pf->used is set to true
   whenever we consume an argument — the outer loop in builtin_printf uses
   that to decide whether to re-run the format against the remaining args. */
void	pf_conv(t_pf *pf, t_spec *sp, char conv)
{
	char		fmt[80];
	const char	*arg;

	if (conv == '%')
		return ((void)vec_push_char(pf->out, '%'));
	pf->used = true;
	arg = pf_arg(pf);
	if (conv == 'c')
		return (pf_conv_char(pf, sp, arg));
	if (conv == 'q')
		return (pf_conv_quote(pf, sp, arg));
	if (conv == 'b')
	{
		if (!arg)
			arg = "";
		return (pf_emit_b_padded(pf, sp, arg));
	}
	pf_build_spec(fmt, sp, conv);
	pf_emit_sized(pf, sp, fmt, arg);
}
