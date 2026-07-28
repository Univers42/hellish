/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_mapfile.c                                  :+:      :+:    :+:   */
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

/* mapfile / readarray [-t] [ARR]: read every line of stdin into the
   indexed array ARR (default MAPFILE). -t strips the trailing newline of
   each line. This is THE idiom for "slurp a file into an array"; without
   it, iterating file lines forces the read-loop-in-subshell dance. Runs
   in-parent for `mapfile a < f`; inside a pipeline it forks (bash-correct:
   `cmd | mapfile a` does not populate the parent, matching lastpipe-off). */

/* Read all of fd 0 into one growable buffer. */
static char	*slurp_stdin(void)
{
	t_string	buf;
	char		chunk[4096];
	ssize_t		n;

	vec_init(&buf);
	buf.elem_size = 1;
	n = read(0, chunk, sizeof(chunk));
	while (n > 0)
	{
		vec_push_nstr(&buf, chunk, (size_t)n);
		n = read(0, chunk, sizeof(chunk));
	}
	vec_push_char(&buf, '\0');
	return ((char *)buf.ctx);
}

/* Duplicate n bytes of `line` into a fresh element appended to `elems`. */
static void	push_line(t_vec *elems, const char *line, size_t n)
{
	char	*el;

	el = ft_strndup(line, n);
	vec_push(elems, &el);
}

/* Split `text` into lines, one array element each. With keep_nl the
   terminating '\n' stays on the element (bash without -t); otherwise it
   is dropped. A trailing partial line (no newline) still becomes an
   element; a wholly empty input yields an empty array. */
static char	*mapfile_encode(const char *text, int keep_nl)
{
	t_vec	elems;
	char	*el;
	size_t	i;
	size_t	start;

	vec_init(&elems);
	elems.elem_size = sizeof(char *);
	i = 0;
	start = 0;
	while (text[i])
	{
		if (text[i] == '\n')
		{
			push_line(&elems, text + start, i - start + keep_nl);
			start = i + 1;
		}
		i++;
	}
	if (i > start)
		push_line(&elems, text + start, i - start);
	el = arr_from_elems((char **)elems.ctx, (int)elems.len, NULL);
	return (mapfile_free_elems(&elems), el);
}

/* Free the element vector built by mapfile_encode. */
void	mapfile_free_elems(t_vec *elems)
{
	size_t	i;

	i = 0;
	while (i < elems->len)
		xfree(((char **)elems->ctx)[i++]);
	xfree(elems->ctx);
}

int	builtin_mapfile(t_shell *state, t_vec argv)
{
	char	*text;
	char	*val;
	char	*name;
	int		keep_nl;
	size_t	i;

	keep_nl = 1;
	name = "MAPFILE";
	i = 1;
	while (i < argv.len)
	{
		if (ft_strcmp(((char **)argv.ctx)[i], "-t") == 0)
			keep_nl = 0;
		else if (((char **)argv.ctx)[i][0] != '-')
			name = ((char **)argv.ctx)[i];
		i++;
	}
	text = slurp_stdin();
	val = mapfile_encode(text, keep_nl);
	xfree(text);
	env_set(&state->env, env_create(ft_strdup(name), val, false));
	return (0);
}
