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
#include "sys.h"

char	*hd_delim(t_ltoken *t, char *base);
void	skip_one_body(const char **p, size_t *line, t_hd *s);

/* Does this heredoc operator token ask for tab stripping (`<<-`)?  The token
   of `3<<-EOF` starts at the fd digits, so they are skipped first: every
   check that compared the token's first bytes with "<<-" missed the dash
   whenever an fd was given, and the tab-indented delimiter never matched. */
bool	heredoc_op_strips(const char *op)
{
	while (op && ft_isdigit((unsigned char)*op))
		op++;
	return (op && ft_strncmp(op, STRIP_HEREDOC, 3) == 0);
}

/* Record one heredoc spec (line + literal delimiter) from a `<<word` pair.
   `<<-` is read from the operator's LAST byte: `3<<-EOF` starts with the fd
   digits, so looking at byte 2 missed the dash, the tab-indented delimiter
   never matched, and the body swallowed the rest of the script. */
static void	push_spec(t_vec *v, t_deque_tok *tt, size_t i, size_t line)
{
	t_hd		sp;
	t_ltoken	*op;

	op = (t_ltoken *)deque_idx(&tt->deqtok, i);
	sp.dash = (op->len >= 3 && (tt->base + op->off)[op->len - 1] == '-');
	sp.line = line;
	sp.delim = hd_delim((t_ltoken *)deque_idx(&tt->deqtok, i + 1), tt->base);
	vec_push(v, &sp);
}

/* Append a spec for every `<<word` among tt's tokens, all tagged `line`. */
static int	push_specs(t_deque_tok *tt, size_t line, t_vec *v)
{
	size_t	i;
	int		found;

	i = 0;
	found = 0;
	while (i + 1 < tt->deqtok.len)
	{
		if (((t_ltoken *)deque_idx(&tt->deqtok, i))->tt == TT_HEREDOC)
			(push_spec(v, tt, i, line), found++);
		i++;
	}
	return (found);
}

/* Tokenise the command line at *p -- grown across physical lines while a
   quote or substitution is open (extract5.c) -- append a spec for every
   `<<word` on it, and move *p past it. *line ends on the index of its last
   physical line, the one its bodies follow. Returns the operator count. */
int	specs_on_line(const char **p, size_t *line, t_vec *v)
{
	t_deque_tok	tt;
	const char	*end;
	char		*copy;
	int			found;

	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 16, sizeof(t_ltoken));
	copy = hd_lex_line(*p, &end, line, &tt);
	found = 0;
	if (copy)
		found = push_specs(&tt, *line, v);
	*p = end;
	return (xfree(tt.deqtok.buff), xfree(copy), found);
}

/* Walk the source command line by command line, collecting heredoc specs and
   skipping each heredoc's body so its bytes are never lexed. *out receives
   the spec array. */
int	collect_specs(const char *str, t_hd **out)
{
	t_vec		v;
	const char	*p;
	size_t		line;
	int			k;

	vec_init(&v);
	v.elem_size = sizeof(t_hd);
	p = str;
	line = 0;
	while (*p)
	{
		k = specs_on_line(&p, &line, &v);
		line++;
		while (k-- > 0 && *p)
			skip_one_body(&p, &line, &((t_hd *)v.ctx)[v.len - 1 - k]);
	}
	return (*out = (t_hd *)v.ctx, (int)v.len);
}
