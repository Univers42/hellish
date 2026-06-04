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

/* "%[flags][width][.prec]" + "ll" for integers + conv for snprintf. */
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

static void	pf_conv_str(char *fmt, const char *arg, char conv, char *buf)
{
	if (conv == 's')
	{
		if (arg)
			snprintf(buf, 4096, fmt, arg);
		else
			snprintf(buf, 4096, fmt, "");
	}
	else if (ft_strchr("diouxX", conv))
		snprintf(buf, 4096, fmt, pf_to_num(arg));
	else if (ft_strchr("feEgGaA", conv))
	{
		if (arg)
			snprintf(buf, 4096, fmt, atof(arg));
		else
			snprintf(buf, 4096, fmt, 0.0);
	}
}

void	pf_conv(t_pf *pf, const char *spec, int speclen, char conv)
{
	char		buf[4096];
	char		fmt[80];
	const char	*arg;

	if (conv == '%')
		return ((void)vec_push_char(pf->out, '%'));
	pf->used = true;
	arg = pf_next_arg(pf);
	if (conv == 'c')
	{
		if (arg)
			return ((void)vec_push_char(pf->out, arg[0]));
		return ((void)vec_push_char(pf->out, '\0'));
	}
	if (conv == 'b')
	{
		if (arg)
			return (pf_emit_b(pf->out, arg, &pf->stop));
		return (pf_emit_b(pf->out, "", &pf->stop));
	}
	pf_build_spec(fmt, spec, speclen, conv);
	buf[0] = '\0';
	pf_conv_str(fmt, arg, conv, buf);
	vec_push_str(pf->out, buf);
}

/* Skip the flags / width / precision between '%' and the conversion char. */
void	pf_scan_spec(const char *fmt, int *i)
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
