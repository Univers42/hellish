/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read5.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"
#include <unistd.h>

/* Which option letters take the NEXT word as their value. Getting this set
   wrong is not a missing feature, it is silent corruption: an unlisted
   value-taker leaves its argument looking like the first variable name, so
   `read -n 2 v` used to assign the line to a variable called `2` and leave
   `v` untouched -- no message, status 0. Every letter bash gives an operand
   to belongs here even when the option itself is not implemented. */
static bool	rd_takes_value(char c)
{
	return (c && ft_strchr("apndtNui", c) != NULL);
}

/* Apply one value-taking option. -u (read from another fd) and -i (preload
   the editing buffer) are consumed and ignored: consuming is what stops the
   corruption above, and neither is advertised by `help read`. */
static void	rd_set_opt(t_rdopt *o, char c, char *val)
{
	if (c == 'a')
		o->aname = val;
	else if (c == 'p')
		o->prompt = val;
	else if (c == 'd')
		o->delim = *val;
	else if (c == 't')
		o->tmo_ms = rd_secs_ms(val);
	else if (c == 'n' || c == 'N')
	{
		o->nchars = ft_atol(val);
		o->exact = (c == 'N');
	}
}

/* read option scanner, generation 3: -r as before, plus the value-taking
   options. Value-takers grab the following argument in the order their
   letters appear, so `read -rn 2 v` and `read -rp "> " v` both work like
   bash. Unknown letters are still ignored silently. */
static size_t	take_values(t_vec argv, size_t i, const char *word, t_rdopt *o)
{
	int	j;

	j = 1;
	while (word[j])
	{
		if (word[j] == 'r')
			o->raw = true;
		if (rd_takes_value(word[j]) && i + 1 < argv.len)
		{
			i++;
			rd_set_opt(o, word[j], ((char **)argv.ctx)[i]);
		}
		j++;
	}
	return (i);
}

size_t	parse_read_opts2(t_vec argv, t_rdopt *o)
{
	size_t	i;
	char	*w;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		w = ((char **)argv.ctx)[i];
		i = take_values(argv, i, w, o);
		i++;
	}
	return (i);
}

/* read -a NAME: every IFS field becomes one element; an empty line
   yields an empty array, exactly like bash. */
void	rd_assign_array(t_shell *state, char *line, t_rdopt *o)
{
	t_vec	elems;
	char	*p;
	char	*val;
	size_t	i;

	vec_init(&elems);
	elems.elem_size = sizeof(char *);
	p = line;
	while (is_ifs_ws(*p, o->ifs))
		p++;
	while (*p)
	{
		val = next_field(&p, o->ifs, o->raw);
		vec_push(&elems, &val);
		skip_delim(&p, o->ifs);
	}
	val = arr_from_elems((char **)elems.ctx, (int)elems.len, NULL);
	env_set(&state->env, env_create(ft_strdup(o->aname), val, false));
	i = 0;
	while (i < elems.len)
		xfree(((char **)elems.ctx)[i++]);
	xfree(elems.ctx);
}
