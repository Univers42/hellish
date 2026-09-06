/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_mapfile2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* An operand: the first one names the array, later ones are ignored, as
   bash does. */
static void	mf_operand(t_mfopt *o, const char *w)
{
	if (!o->named)
	{
		o->named = true;
		o->name = (char *)w;
	}
}

/* Digits only, at least one: the counts, the origin and the fd. */
static bool	mf_number(const char *s)
{
	if (!*s)
		return (false);
	while (*s)
	{
		if (!ft_isdigit((unsigned char)*s))
			return (false);
		s++;
	}
	return (true);
}

/* Apply one option letter with its value (NULL when the word ended and no
   argument followed).  -C and -c are recognised so their values are never
   mistaken for the array name, then refused: a callback that silently
   never runs would be worse than an error.  Returns non-zero after a
   message, with o->err the status to exit with. */
static int	mf_set(t_shell *state, t_mfopt *o, char c, const char *val)
{
	o->err = 2;
	if (!ft_strchr("dnOsuCc", c))
		return (ft_eprintf("%s: mapfile: -%c: invalid option\n",
				state->ctx, c), 1);
	if (!val)
		return (ft_eprintf("%s: mapfile: -%c: option requires an argument\n",
				state->ctx, c), 1);
	if (c == 'C' || c == 'c')
		return (ft_eprintf("%s: mapfile: -%c: callbacks are not supported\n",
				state->ctx, c), 1);
	o->err = 1;
	if (c == 'd')
		o->delim = *val;
	else if (!mf_number(val))
		return (ft_eprintf("%s: mapfile: %s: invalid number\n",
				state->ctx, val), 1);
	else if (c == 'n')
		o->max = ft_atol(val);
	else if (c == 'O')
		o->origin = ft_atol(val);
	else if (c == 's')
		o->skip = ft_atol(val);
	else if (c == 'u')
		o->fd = ft_atoi(val);
	return (o->err = 0, 0);
}

/* One -xyz word.  -t may stack; the first value-taker ENDS the word
   (getopt): its value is the rest of the word, or the next argument when
   that is empty -- `-d ''` is how bash spells the NUL delimiter.  Returns
   the index of the last argument consumed, -1 after a message. */
static long	mf_word(t_shell *state, t_vec argv, size_t i, t_mfopt *o)
{
	const char	*w;
	const char	*val;
	int			j;

	w = ((char **)argv.ctx)[i];
	j = 1;
	while (w[j] == 't')
	{
		o->strip = true;
		j++;
	}
	if (!w[j])
		return ((long)i);
	val = w + j + 1;
	if (!*val && ft_strchr("dnOsuCc", w[j]))
	{
		val = NULL;
		if (i + 1 < argv.len)
			val = ((char **)argv.ctx)[++i];
	}
	if (mf_set(state, o, w[j], val))
		return (-1);
	return ((long)i);
}

/* Parse argv into o.  `--` ends the options.  Returns 0, or the status to
   exit with after mf_set's message. */
int	mapfile_parse(t_shell *state, t_vec argv, t_mfopt *o)
{
	const char	*w;
	size_t		i;
	long		r;

	*o = (t_mfopt){.delim = '\n', .origin = -1, .name = "MAPFILE"};
	i = 1;
	while (i < argv.len)
	{
		w = ((char **)argv.ctx)[i];
		if (ft_strcmp(w, "--") == 0)
		{
			if (i + 1 < argv.len)
				mf_operand(o, ((char **)argv.ctx)[i + 1]);
			return (0);
		}
		r = i;
		if (w[0] == '-' && w[1])
			r = mf_word(state, argv, i, o);
		else
			mf_operand(o, w);
		if (r < 0)
			return (o->err);
		i = (size_t)r + 1;
	}
	return (0);
}
