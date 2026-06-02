/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/*   Created: 2026/03/15 02:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 02:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

typedef struct s_rdopt
{
	char	*ifs;
	bool	raw;
	size_t	first;
}	t_rdopt;

/* IFS value, copied so env mutations (env_set) cannot invalidate it. Unset
   IFS defaults to <space><tab><newline> per POSIX. */
static char	*dup_ifs(t_shell *state)
{
	char	*v;

	v = env_expand(state, "IFS");
	if (!v)
		return (ft_strdup(" \t\n"));
	return (ft_strdup(v));
}

static int	is_ifs(char c, const char *ifs)
{
	return (c && ft_strchr(ifs, c) != NULL);
}

static int	is_ifs_ws(char c, const char *ifs)
{
	return ((c == ' ' || c == '\t' || c == '\n') && is_ifs(c, ifs));
}

/* Read one logical line. In non-raw mode a backslash escapes the next byte and
   a backslash-newline is a line continuation (both removed); other escapes are
   left in the buffer for the splitter. Returns NULL only at EOF with no data. */
static char	*read_one_line(bool raw, int *eof)
{
	char		ch;
	t_string	buf;
	ssize_t		n;
	bool		bs;

	vec_init(&buf);
	buf.elem_size = 1;
	bs = false;
	n = read(STDIN_FILENO, &ch, 1);
	while (n > 0 && !(ch == '\n' && !(bs && !raw)))
	{
		if (ch == '\n')
			(buf.len--, bs = false);
		else
			(vec_push(&buf, &ch), bs = (!raw && ch == '\\' && !bs));
		n = read(STDIN_FILENO, &ch, 1);
	}
	*eof = (n <= 0);
	if (n <= 0 && buf.len == 0)
		return (free(buf.ctx), NULL);
	ch = '\0';
	return (vec_push(&buf, &ch), (char *)buf.ctx);
}

/* Copy one field, processing backslash escapes (non-raw), stopping at the
   first IFS byte. Advances *pp to that byte (or the terminating NUL). */
static char	*next_field(char **pp, const char *ifs, bool raw)
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
	return (vec_push(&f, &nul), (char *)f.ctx);
}

/* Consume the delimiter between fields: any IFS whitespace, then at most one
   non-whitespace IFS byte, then any IFS whitespace. */
static void	skip_delim(char **pp, const char *ifs)
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

/* The last variable receives the remainder verbatim (escapes processed) with
   trailing IFS whitespace stripped. */
static char	*last_field(char *p, const char *ifs, bool raw)
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
	return (vec_push(&f, &nul), (char *)f.ctx);
}

static void	set_var(t_shell *state, char *name, char *value_owned)
{
	env_set(&state->env, env_create(ft_strdup(name), value_owned, false));
}

static void	assign_words(t_shell *state, char *line, t_vec argv, t_rdopt *o)
{
	size_t	i;
	char	*p;

	p = line;
	while (is_ifs_ws(*p, o->ifs))
		p++;
	i = o->first;
	while (i + 1 < argv.len)
	{
		set_var(state, ((char **)argv.ctx)[i], next_field(&p, o->ifs, o->raw));
		skip_delim(&p, o->ifs);
		i++;
	}
	set_var(state, ((char **)argv.ctx)[i], last_field(p, o->ifs, o->raw));
}

static size_t	parse_opts(t_vec argv, bool *raw)
{
	size_t	i;
	int		j;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		j = 1;
		while (((char **)argv.ctx)[i][j])
		{
			if (((char **)argv.ctx)[i][j] == 'r')
				*raw = true;
			j++;
		}
		i++;
	}
	return (i);
}

int	builtin_read(t_shell *state, t_vec argv)
{
	char	*line;
	t_rdopt	o;
	int		eof;

	o.raw = false;
	o.first = parse_opts(argv, &o.raw);
	o.ifs = dup_ifs(state);
	line = read_one_line(o.raw, &eof);
	if (!line)
		return (free(o.ifs), 1);
	if (o.first >= argv.len)
		set_var(state, "REPLY", line);
	else
		(assign_words(state, line, argv, &o), free(line));
	return (free(o.ifs), eof != 0);
}
