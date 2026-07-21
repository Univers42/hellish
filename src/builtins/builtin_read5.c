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

/* read option scanner, generation 2: -r as before, plus the value-taking
   options -a NAME (split every field into an indexed array) and
   -p PROMPT (print to stderr before reading when stdin is a terminal).
   Value-takers grab the following argument in the order their letters
   appear, so `read -rp "> " v` works like bash. Unknown letters are
   still ignored silently (better than eating them as variable names). */
static size_t	take_values(t_vec argv, size_t i, const char *word, t_rdopt *o)
{
	int	j;

	j = 1;
	while (word[j])
	{
		if (word[j] == 'r')
			o->raw = true;
		if ((word[j] == 'a' || word[j] == 'p') && i + 1 < argv.len)
		{
			i++;
			if (word[j] == 'a')
				o->aname = ((char **)argv.ctx)[i];
			else
				o->prompt = ((char **)argv.ctx)[i];
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
