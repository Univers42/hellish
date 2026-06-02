/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extract.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Up-front heredoc-body extraction for string execution (command
   substitution, eval, source). The normal REPL reads heredoc bodies from the
   live input stream after parsing, but a string is tokenised whole, so its
   body lines would otherwise be parsed as commands. Here we tokenise once to
   locate <<word operators (robust against the `<<` arithmetic shift), pull each
   body out of the string into a separate stream (state->hd_src) and leave a
   body-stripped string for the parser. */

#include "lexer.h"
#include "redir.h"
#include "libft.h"

typedef struct s_hd
{
	char	*delim;
	bool	dash;
	size_t	line;
}	t_hd;

static size_t	line_of(const char *str, const char *p)
{
	size_t	ln;

	ln = 0;
	while (str < p)
		if (*str++ == '\n')
			ln++;
	return (ln);
}

/* Literal delimiter of a heredoc: drop quote characters and one level of
   backslash escaping (`<<'EOF'`, `<<"EOF"`, `<<\EOF` all delimit on EOF). */
static char	*hd_delim(t_token *t)
{
	char	*d;
	int		i;
	int		k;

	d = malloc(t->len + 1);
	if (!d)
		return (NULL);
	i = 0;
	k = 0;
	while (i < t->len)
	{
		if (t->start[i] == '\'' || t->start[i] == '"')
			i++;
		else if (t->start[i] == '\\' && i + 1 < t->len)
			(i++, d[k++] = t->start[i++]);
		else
			d[k++] = t->start[i++];
	}
	d[k] = '\0';
	return (d);
}

static int	collect_specs(const char *str, t_deque_tok *tt, t_hd **out)
{
	t_vec		v;
	t_hd		sp;
	t_token		*tok;
	size_t		i;

	vec_init(&v);
	v.elem_size = sizeof(t_hd);
	i = 0;
	while (i + 1 < tt->deqtok.len)
	{
		tok = (t_token *)deque_idx(&tt->deqtok, i++);
		if (tok->tt != TT_HEREDOC)
			continue ;
		sp.dash = (tok->len >= 3 && tok->start[2] == '-');
		sp.line = line_of(str, tok->start);
		sp.delim = hd_delim((t_token *)deque_idx(&tt->deqtok, i));
		vec_push(&v, &sp);
	}
	*out = (t_hd *)v.ctx;
	return ((int)v.len);
}

static bool	is_delim_line(const char *line, size_t len, t_hd *s)
{
	size_t	i;
	size_t	dl;

	if (len > 0 && line[len - 1] == '\n')
		len--;
	i = 0;
	if (s->dash)
		while (i < len && line[i] == '\t')
			i++;
	dl = ft_strlen(s->delim);
	return (len - i == dl && ft_strncmp(line + i, s->delim, dl) == 0);
}

/* Pull body lines (and the closing delimiter line) for one heredoc out of the
   source, appending them to `body` and advancing *p / *cur. */
static void	collect_body(const char **p, size_t *cur, t_string *body, t_hd *s)
{
	const char	*ls;
	size_t		blen;

	while (**p)
	{
		ls = *p;
		while (**p && **p != '\n')
			(*p)++;
		if (**p == '\n')
			(*p)++;
		blen = *p - ls;
		(*cur)++;
		vec_push_nstr(body, ls, blen);
		if (is_delim_line(ls, blen, s))
			break ;
	}
}

static void	free_specs(t_hd *sp, int n)
{
	int	i;

	i = 0;
	while (i < n)
		free(sp[i++].delim);
	free(sp);
}

/* Is the delimiter line reachable from p without consuming it? Used to leave
   an as-yet-unterminated heredoc (a simple top-level "cat <<EOF" whose body the
   REPL still reads from the live stream) untouched. */
static bool	delim_present(const char *p, t_hd *s)
{
	const char	*ls;

	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		if (*p == '\n')
			p++;
		if (is_delim_line(ls, p - ls, s))
			return (true);
	}
	return (false);
}

/* out[0] = body-stripped source for the parser, out[1] = concatenated heredoc
   bodies (each terminated by its delimiter line) in source order. Only
   heredocs whose delimiter is present are extracted; returns the count. */
static int	walk_and_strip(const char *str, t_hd *sp, int n, t_string *out)
{
	const char	*p;
	const char	*ls;
	size_t		cur;
	size_t		intro;
	int			si;
	int			got;

	p = str;
	cur = 0;
	si = 0;
	got = 0;
	while (*p)
	{
		intro = cur;
		ls = p;
		while (*p && *p != '\n')
			p++;
		if (*p == '\n')
			p++;
		vec_push_nstr(&out[0], ls, p - ls);
		cur++;
		while (si < n && sp[si].line < intro)
			si++;
		while (si < n && sp[si].line == intro)
		{
			if (delim_present(p, &sp[si]))
				(collect_body(&p, &cur, &out[1], &sp[si]), got++);
			si++;
		}
	}
	return (got);
}

bool	split_heredocs(const char *str, char **stripped, char **bodies)
{
	t_deque_tok	tt;
	t_hd		*sp;
	t_string	out[2];
	int			n;
	int			got;

	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 100, sizeof(t_token));
	tokenizer((char *)str, &tt);
	n = collect_specs(str, &tt, &sp);
	free(tt.deqtok.buff);
	if (n == 0)
		return (free(sp), false);
	vec_init(&out[0]);
	vec_init(&out[1]);
	out[0].elem_size = 1;
	out[1].elem_size = 1;
	got = walk_and_strip(str, sp, n, out);
	free_specs(sp, n);
	if (got == 0)
		return (free(out[0].ctx), free(out[1].ctx), false);
	*stripped = (char *)out[0].ctx;
	*bodies = (char *)out[1].ctx;
	return (true);
}
