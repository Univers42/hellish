/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extract3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/04 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Heredoc-operator detection that is body-aware: tokenising the whole source
   at once (the old approach) mis-counts operators when a heredoc BODY holds
   shell-significant bytes (an unterminated quote in a vimrc/syslog config line
   swallows the next `<<word`). We instead tokenise one COMMAND line at a time
   and skip each heredoc's body before scanning on, so body text is never
   lexed as shell — mirroring how the parser consumes heredocs. */

#include "heredoc_private.h"
#include "lexer.h"
#include "redir.h"

char	*hd_delim(t_token *t);
void	skip_one_body(const char **p, size_t *line, t_hd *s);

/* Record one heredoc spec (line + literal delimiter) from a `<<word` pair. */
static void	push_spec(t_vec *v, t_deque_tok *tt, size_t i, size_t line)
{
	t_hd	sp;

	sp.dash = false;
	if (((t_token *)deque_idx(&tt->deqtok, i))->len >= 3
		&& ((t_token *)deque_idx(&tt->deqtok, i))->start[2] == '-')
		sp.dash = true;
	sp.line = line;
	sp.delim = hd_delim((t_token *)deque_idx(&tt->deqtok, i + 1));
	vec_push(v, &sp);
}

/* Tokenise a single command line (copied so its NUL terminates the lexer) and
   append a spec for every `<<word` on it; return the operator count. */
static int	specs_on_line(const char *ls, size_t len, size_t line, t_vec *v)
{
	t_deque_tok	tt;
	char		*copy;
	size_t		i;
	int			found;

	copy = ft_strndup(ls, len);
	if (!copy)
		return (0);
	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 16, sizeof(t_token));
	tokenizer(copy, &tt);
	i = 0;
	found = 0;
	while (i + 1 < tt.deqtok.len)
	{
		if (((t_token *)deque_idx(&tt.deqtok, i))->tt == TT_HEREDOC)
			(push_spec(v, &tt, i, line), found++);
		i++;
	}
	return (xfree(tt.deqtok.buff), xfree(copy), found);
}

/* Walk the source line by line, collecting heredoc specs and skipping each
   heredoc's body so its bytes are never lexed. *out receives the spec array. */
int	collect_specs(const char *str, t_hd **out)
{
	t_vec		v;
	const char	*p;
	const char	*ls;
	size_t		line;
	int			k;

	vec_init(&v);
	v.elem_size = sizeof(t_hd);
	p = str;
	line = 0;
	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		k = specs_on_line(ls, p - ls + (*p == '\n'), line++, &v);
		p += (*p == '\n');
		while (k-- > 0 && *p)
			skip_one_body(&p, &line, &((t_hd *)v.ctx)[v.len - 1 - k]);
	}
	return (*out = (t_hd *)v.ctx, (int)v.len);
}
