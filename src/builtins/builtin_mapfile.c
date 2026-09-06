/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_mapfile.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* mapfile / readarray [-d delim] [-n n] [-O idx] [-s n] [-t] [-u fd] [ARR]:
   read a descriptor (stdin by default) into the indexed array ARR (default
   MAPFILE), one element per delimiter-terminated record.  THE idiom for
   "slurp a file into an array"; without it, iterating file lines forces
   the read-loop-in-subshell dance.  Runs in-parent for `mapfile a < f`;
   inside a pipeline it forks (bash-correct: `cmd | mapfile a` does not
   populate the parent, matching lastpipe-off).

   Generation 1 knew -t and the name and nothing else, and read fd 0
   whatever -u said -- issue #122 found `-d ''` reading one element where
   bash reads a NUL-separated list.  Options live in builtin_mapfile2.c. */

/* Read all of fd into buf, the length kept beside the bytes: with -d ''
   the records are NUL-terminated, so the text cannot be a C string.
   False on a read error, which is how a -u fd that is not open shows. */
static bool	slurp_fd(int fd, t_string *buf)
{
	char	chunk[4096];
	ssize_t	n;

	vec_init(buf);
	buf->elem_size = 1;
	n = read(fd, chunk, sizeof(chunk));
	while (n > 0)
	{
		vec_push_nstr(buf, chunk, (size_t)n);
		n = read(fd, chunk, sizeof(chunk));
	}
	return (n == 0);
}

/* Free the element vector built by mapfile_split. */
void	mapfile_free_elems(t_vec *elems)
{
	size_t	i;

	i = 0;
	while (i < elems->len)
		xfree(((char **)elems->ctx)[i++]);
	xfree(elems->ctx);
}

/* Cut buf into records on the delimiter.  The first o->skip records are
   discarded and at most o->max (0 = all) are kept; -t drops the
   terminating delimiter, otherwise it stays on the element as in bash.  A
   trailing partial record still counts; an empty input yields none. */
static void	mapfile_split(t_string *buf, t_mfopt *o, t_vec *elems)
{
	const char	*p;
	char		*el;
	size_t		start;
	size_t		i;
	long		seen;

	p = (const char *)buf->ctx;
	start = 0;
	seen = 0;
	while (start < buf->len && (o->max == 0 || (long)elems->len < o->max))
	{
		i = start;
		while (i < buf->len && p[i] != o->delim)
			i++;
		if (seen++ >= o->skip)
		{
			el = ft_strndup(p + start, i - start
					+ (i < buf->len && !o->strip));
			vec_push(elems, &el);
		}
		start = i + 1;
	}
}

/* Assign the records.  Without -O the array is replaced from index 0;
   with it the existing elements stay and the records land from the origin
   up -- bash's "array_flush unless -O".  A scalar in the way is promoted
   to element 0 by arr_with_set, which is bash's rule too. */
static void	mapfile_assign(t_shell *state, t_mfopt *o, t_vec *elems)
{
	char	*val;
	char	*next;
	char	*old;
	size_t	k;

	if (o->origin < 0)
		val = arr_from_elems((char **)elems->ctx, (int)elems->len, NULL);
	else
	{
		old = env_expand(state, o->name);
		if (old)
			val = ft_strdup(old);
		else
			val = arr_from_elems(NULL, 0, NULL);
		k = 0;
		while (k < elems->len)
		{
			next = arr_with_set(val, o->origin + (long)k,
					((char **)elems->ctx)[k]);
			xfree(val);
			val = next;
			k++;
		}
	}
	env_set(&state->env, env_create(ft_strdup(o->name), val, false));
}

int	builtin_mapfile(t_shell *state, t_vec argv)
{
	t_mfopt		o;
	t_string	buf;
	t_vec		elems;
	int			st;

	st = mapfile_parse(state, argv, &o);
	if (st)
		return (st);
	if (!slurp_fd(o.fd, &buf))
	{
		ft_eprintf("%s: mapfile: %d: invalid file descriptor: %s\n",
			state->ctx, o.fd, strerror(errno));
		return (xfree(buf.ctx), 1);
	}
	vec_init(&elems);
	elems.elem_size = sizeof(char *);
	mapfile_split(&buf, &o, &elems);
	mapfile_assign(state, &o, &elems);
	xfree(buf.ctx);
	mapfile_free_elems(&elems);
	return (0);
}
