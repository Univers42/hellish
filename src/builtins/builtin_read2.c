/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

/* Copy one field, stopping at the first IFS byte. */
char	*next_field(char **pp, const char *ifs, bool raw)
{
	t_string	f;
	char		*p;
	char		nul;

	vec_init(&f);
	f.elem_size = 1;
	p = *pp;
	while (*p && !is_ifs(*p, ifs))
	{
		if (!raw && *p == '\\' && p[1])
			p++;
		vec_push(&f, p);
		p++;
	}
	*pp = p;
	nul = '\0';
	vec_push(&f, &nul);
	return ((char *)f.ctx);
}

/* Consume the delimiter between fields. */
void	skip_delim(char **pp, const char *ifs)
{
	char	*p;

	p = *pp;
	while (is_ifs_ws(*p, ifs))
		p++;
	if (is_ifs(*p, ifs) && !is_ifs_ws(*p, ifs))
		p++;
	while (is_ifs_ws(*p, ifs))
		p++;
	*pp = p;
}

/* The last variable receives the remainder with trailing IFS stripped. */
char	*last_field(char *p, const char *ifs, bool raw)
{
	t_string	f;
	char		nul;

	vec_init(&f);
	f.elem_size = 1;
	while (*p)
	{
		if (!raw && *p == '\\' && p[1])
			p++;
		vec_push(&f, p);
		p++;
	}
	while (f.len > 0 && is_ifs_ws(((char *)f.ctx)[f.len - 1], ifs))
		f.len--;
	nul = '\0';
	vec_push(&f, &nul);
	return ((char *)f.ctx);
}

void	rd_set_var(t_shell *state, char *name, char *value_owned)
{
	env_set(&state->env, env_create(ft_strdup(name), value_owned, false));
}

void	assign_words(t_shell *state, char *line, t_vec argv, t_rdopt *o)
{
	size_t	i;
	char	*p;

	p = line;
	while (is_ifs_ws(*p, o->ifs))
		p++;
	i = o->first;
	while (i + 1 < argv.len)
	{
		rd_set_var(state, ((char **)argv.ctx)[i],
			next_field(&p, o->ifs, o->raw));
		skip_delim(&p, o->ifs);
		i++;
	}
	rd_set_var(state, ((char **)argv.ctx)[i], last_field(p, o->ifs, o->raw));
}
