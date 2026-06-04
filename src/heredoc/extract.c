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

#include "heredoc_private.h"
#include "lexer.h"
#include "redir.h"

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
		{
			i++;
			d[k++] = t->start[i++];
		}
		else
			d[k++] = t->start[i++];
	}
	d[k] = '\0';
	return (d);
}

int	collect_specs(const char *str, t_deque_tok *tt, t_hd **out)
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

bool	is_delim_line(const char *line, size_t len, t_hd *s)
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
void	collect_body(const char **p, size_t *cur, t_string *body, t_hd *s)
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

/* out[0] = body-stripped source for the parser, out[1] = concatenated
** heredoc bodies (each terminated by its delimiter line) in source order.
** Only heredocs whose delimiter is present are extracted; returns the count. */
