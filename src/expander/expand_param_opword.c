/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_opword.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 18:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 18:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "decomposer.h"

/* One segment: 'q' + text for what the word quoted, 'u' + text for the
   rest. */
static void	seg_push(t_vec *v, char kind, const char *s, int len)
{
	char	*e;

	if (len < 0)
		len = 0;
	e = xmalloc((size_t)len + 2);
	e[0] = kind;
	if (len > 0)
		ft_memcpy(e + 1, s, (size_t)len);
	e[len + 1] = '\0';
	vec_push(v, &e);
}

/* Tilde-expand the word, remembering how long its leading run was and
   where its ~prefix ended (the first '/'), so seg_first can tell the home
   directory from the text after it. pre[0] is -1 when nothing expanded. */
static void	seg_tilde(t_shell *state, t_ast_node *w, int pre[2])
{
	t_token	*first;
	char	*before;

	pre[0] = -1;
	if (!w->children.len)
		return ;
	first = &((t_ast_node *)w->children.ctx)[0].token;
	before = first->start;
	pre[0] = first->len;
	pre[1] = 0;
	while (pre[1] < first->len && first->start[pre[1]] != '/')
		pre[1]++;
	expand_tilde_word(state, w);
	if (first->start == before)
		pre[0] = -1;
}

/* The leading run, after tilde expansion. A home directory is quoted
   (POSIX 2.6.1: ${u:-~} is one field even when HOME holds a blank); the
   text after the ~prefix is not. */
static void	seg_first(t_vec *v, t_token t, int pre[2])
{
	int	home;

	home = t.len - (pre[0] - pre[1]);
	seg_push(v, 'q', t.start, home);
	seg_push(v, 'u', t.start + home, t.len - home);
}

/* The expanded word as segments, encoded as an array value whose magic
   byte is SEG_MAGIC. Quoted text -- '...', "...", $'...', a backslash
   escape -- is 'q'. Everything else is 'u', literal text included: bash
   splits and globs the whole result, so ${u:-a b} is two fields and
   ${u:-*.c} a glob, while ${u:-"a b"} and ${u:-\*} are not. */
static char	*seg_encode(t_ast_node *w, int pre[2])
{
	t_vec	v;
	t_token	*t;
	char	*enc;
	size_t	i;

	vec_init(&v);
	v.elem_size = sizeof(char *);
	i = -1;
	while (++i < w->children.len
		&& ((t_ast_node *)w->children.ctx)[i].node_type == AST_TOKEN)
	{
		t = &((t_ast_node *)w->children.ctx)[i].token;
		if (i == 0 && pre[0] >= 0)
			seg_first(&v, *t, pre);
		else if (t->tt == TT_SQWORD || t->tt == TT_DQWORD
			|| t->tt == TT_DQENVVAR)
			seg_push(&v, 'q', t->start, t->len);
		else
			seg_push(&v, 'u', t->start, t->len);
	}
	enc = arr_from_elems((char **)v.ctx, (int)v.len, NULL);
	enc[0] = SEG_MAGIC;
	while (v.len)
		xfree(((char **)v.ctx)[--v.len]);
	return (xfree(v.ctx), enc);
}

/* Unquoted ${p-w} / ${p:-w} / ${p+w} / ${p:+w} whose word is USED, in a
   split context: park the word's segments behind a marker so the splitter
   (emit_op_segments) splits and globs its unquoted parts and keeps the
   quoted ones whole.

   The token used to become one TT_DQWORD whenever the word's TEXT held no
   unquoted blank, which lost both halves: ${u:-$P} was never split and
   ${u:-*.c} never globbed, and ${u:-"a b"c d} split inside the quotes. */
bool	pf_op_word_segments(t_shell *state, t_token *tt, t_pe_op o)
{
	t_ast_node	w;
	t_token		t;
	char		*enc;
	int			pre[2];

	if (o.wlen <= 0)
		return (false);
	ft_bzero(&t, sizeof(t));
	t.start = (char *)o.word;
	t.len = o.wlen;
	t.tt = TT_WORD;
	w = reparse_word(t, false);
	seg_tilde(state, &w, pre);
	expand_cmd_substitutions(state, &w);
	expand_env_vars(state, &w, false);
	enc = seg_encode(&w, pre);
	free_ast(&w);
	tt->start = arr_mark_push(state, enc, (int)ft_strlen(enc));
	tt->len = (int)ft_strlen(tt->start);
	tt->allocated = false;
	xfree(enc);
	return (true);
}
