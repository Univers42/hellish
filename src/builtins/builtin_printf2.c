/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_printf2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "printf_private.h"

/* Single pass through the format string: handle backslash escapes (passed
   to pf_escape), % conversions (pf_conv), and literal characters. A '\c'
   escape sets pf->stop which aborts both this pass and the outer loop in
   builtin_printf immediately. */
static void	run_format(t_pf *pf, const char *fmt)
{
	int		i;
	int		start;

	i = 0;
	while (fmt[i] && !pf->stop)
	{
		if (fmt[i] == '\\' && fmt[i + 1])
			vec_push_char(pf->out, pf_escape(fmt, &i, &pf->stop));
		else if (fmt[i] == '%')
		{
			start = i++;
			pf_scan_spec(fmt, &i);
			if (!fmt[i])
				return ((void)vec_push_char(pf->out, '%'));
			pf_conv(pf, fmt + start, i - start, fmt[i]);
			i++;
		}
		else
			vec_push_char(pf->out, fmt[i++]);
	}
}

/* The format is argv[1], unless argv[1] is the end-of-options marker "--"
   (as in `printf -- '-'`), in which case the real format is argv[2]. Returns
   the index of the format word so a single leading "--" is consumed, matching
   bash. */
static int	pf_fmt_index(t_vec argv)
{
	if (argv.len >= 2 && !ft_strncmp(((char **)argv.ctx)[1], "--", 3))
		return (2);
	return (1);
}

/* printf format [arguments] : POSIX printf. The format is reused while
   arguments remain and it contains at least one consuming conversion. */
int	builtin_printf(t_shell *state, t_vec argv)
{
	t_pf		pf;
	t_string	out;
	int			fmt_idx;

	fmt_idx = pf_fmt_index(argv);
	if ((int)argv.len <= fmt_idx)
		return (ft_eprintf("%s: printf: usage: printf format [arguments]\n",
				state->ctx), 2);
	vec_init(&out);
	out.elem_size = 1;
	pf = (t_pf){.out = &out, .av = (char **)argv.ctx, .argc = (int)argv.len,
		.argi = fmt_idx + 1, .used = false, .stop = false};
	run_format(&pf, pf.av[fmt_idx]);
	while (!pf.stop && pf.argi < pf.argc && pf.used)
	{
		pf.used = false;
		run_format(&pf, pf.av[fmt_idx]);
	}
	if (out.len && write(STDOUT_FILENO, out.ctx, out.len))
	{
	}
	xfree(out.ctx);
	return (0);
}
