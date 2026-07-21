/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_ps1b.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* \h (short) and \H (full) hostname for PS1. Cached: the box does not
   change its name mid-session, and the prompt renders per command. */
void	ps1_host(t_string *out, char kind)
{
	static char	full[256];
	char		shortname[256];
	char		*dot;

	if (!full[0])
	{
		if (gethostname(full, sizeof(full) - 1) != 0)
			ft_strlcpy(full, "host", sizeof(full));
		full[sizeof(full) - 1] = '\0';
	}
	if (kind == 'H')
		return ((void)vec_push_str(out, full));
	ft_strlcpy(shortname, full, sizeof(shortname));
	dot = ft_strchr(shortname, '.');
	if (dot)
		*dot = '\0';
	vec_push_str(out, shortname);
}

/* $NAME / ${NAME} inside PS1: expanded from the live environment at every
   render, so `PS1='${VIRTUAL_ENV} \w> '` tracks changes without re-sourcing
   the rc file. Special single-char parameters ($?, $$, ...) are left to
   the shell proper — a prompt string wants variables, and bash's promptvars
   behaviour for plain names is what users expect. Invalid syntax keeps the
   dollar literal. */
void	ps1_dollar(t_shell *state, t_string *out, const char *f, int *i)
{
	int		j;
	int		brace;
	char	*val;

	j = *i + 1;
	brace = (f[j] == '{');
	j += brace;
	if (!is_var_name_p1(f[j]))
		return (vec_push_char(out, '$'), (void)(*i += 1));
	*i = j;
	while (is_var_name_p2(f[j]))
		j++;
	val = env_expand_n(state, (char *)f + *i, j - *i);
	if (val)
		vec_push_str(out, val);
	if (brace && f[j] == '}')
		j++;
	*i = j;
}
