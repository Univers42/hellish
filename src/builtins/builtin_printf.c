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

static const char	*pf_next_arg(t_pf *pf)
{
	if (pf->argi < pf->argc)
		return (pf->av[pf->argi++]);
	return (NULL);
}

/* "%[flags][width][.prec]" (without conv) + a forced "ll" for integers + conv,
   so libc snprintf can always take a (long long)/(unsigned long long). */
static void	pf_build_spec(char *dst, const char *spec, int speclen, char conv)
{
	int	k;

	ft_memcpy(dst, spec, speclen);
	dst[speclen] = '\0';
	if (ft_strchr("diouxX", conv))
		ft_strlcat(dst, "ll", 80);
	k = (int)ft_strlen(dst);
	dst[k] = conv;
	dst[k + 1] = '\0';
}

static void	pf_conv(t_pf *pf, const char *spec, int speclen, char conv)
{
	char		buf[4096];
	char		fmt[80];
	const char	*arg;

	if (conv == '%')
		return ((void)vec_push_char(pf->out, '%'));
	pf->used = true;
	arg = pf_next_arg(pf);
	if (conv == 'c')
		return ((void)vec_push_char(pf->out, arg ? arg[0] : '\0'));
	if (conv == 'b')
		return (pf_emit_b(pf->out, arg ? arg : "", &pf->stop));
	pf_build_spec(fmt, spec, speclen, conv);
	if (conv == 's')
		snprintf(buf, sizeof(buf), fmt, arg ? arg : "");
	else if (ft_strchr("diouxX", conv))
		snprintf(buf, sizeof(buf), fmt, pf_to_num(arg));
	else if (ft_strchr("feEgGaA", conv))
		snprintf(buf, sizeof(buf), fmt, arg ? atof(arg) : 0.0);
	else
		return ;
	vec_push_str(pf->out, buf);
}

/* Skip the flags / width / precision between '%' and the conversion char. */
static void	pf_scan_spec(const char *fmt, int *i)
{
	while (fmt[*i] && ft_strchr("-+ #0", fmt[*i]))
		(*i)++;
	while (ft_isdigit(fmt[*i]))
		(*i)++;
	if (fmt[*i] == '.')
	{
		(*i)++;
		while (ft_isdigit(fmt[*i]))
			(*i)++;
	}
}

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

/* printf format [arguments] : POSIX printf. The format is reused while
   arguments remain and it contains at least one consuming conversion. */
int	builtin_printf(t_shell *state, t_vec argv)
{
	t_pf		pf;
	t_string	out;

	if (argv.len < 2)
		return (ft_eprintf("%s: printf: usage: printf format [arguments]\n",
				state->ctx), 2);
	vec_init(&out);
	out.elem_size = 1;
	pf = (t_pf){.out = &out, .av = (char **)argv.ctx,
		.argc = (int)argv.len, .argi = 2, .used = false, .stop = false};
	run_format(&pf, pf.av[1]);
	while (!pf.stop && pf.argi < pf.argc && pf.used)
	{
		pf.used = false;
		run_format(&pf, pf.av[1]);
	}
	if (out.len && write(STDOUT_FILENO, out.ctx, out.len))
	{
	}
	free(out.ctx);
	return (0);
}
