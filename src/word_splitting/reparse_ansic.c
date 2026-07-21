/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_ansic.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"
#include "parena.h"

/* $'...' ANSI-C quoting. The region was scanned atomically by the lexer
   (advance_ansic); here the escapes are DECODED into a fresh buffer and
   the result becomes a single TT_SQWORD subtoken — quoted semantics: no
   field splitting, no globbing, exactly like bash. Escape set matches
   bash: \n \t \r \a \b \e \E \f \v \\ \' \" \? \nnn \xHH \uHHHH
   \UHHHHHHHH \cX; unknown escapes keep the backslash and char. */

/* Wrap the decoded buffer in an AST_TOKEN child. allocated=true routes
   the buffer through parena_free at teardown: a no-op for the arena copy
   made during a cycle parse, a real free for eval/source heap parses. */
static void	push_ansic_node(t_ast_node *ret, char *buf, int n)
{
	t_ast_node	node;

	node = create_node_tok(AST_TOKEN, create_tok4(buf, n, TT_SQWORD, true));
	vec_init(&node.children);
	node.children.elem_size = sizeof(t_ast_node);
	ast_push_child(ret, &node);
}

/* Decode one backslash escape at a->i (which sits on the backslash).
   Priority: the single-char map, then numeric forms, then control-char
   \cX, then literal passthrough of unknown escapes. */
void	ansic_escape(t_ansic *a)
{
	char	c;
	int		v;

	c = a->s[a->i + 1];
	v = ansic_simple(c);
	if (v >= 0)
	{
		a->dst[a->n++] = (char)v;
		a->i += 2;
		return ;
	}
	if (c == 'x' || c == 'u' || c == 'U')
		return (ansic_num(a, c));
	if (c >= '0' && c <= '7')
		return (ansic_num(a, 'o'));
	if (c == 'c' && a->i + 2 < a->len)
	{
		a->dst[a->n++] = (char)(ft_toupper(a->s[a->i + 2]) ^ 64);
		a->i += 3;
		return ;
	}
	a->dst[a->n++] = '\\';
	a->dst[a->n++] = c;
	a->i += 2;
}

/* Reparser entry, *i on the '$'. The decode buffer can never outgrow the
   raw region (every escape shrinks or keeps length), so one arena block
   of the token's length is always enough. */
void	reparse_ansic(t_ast_node *ret, int *i, t_token t)
{
	t_ansic	a;

	ft_assert(t.start[*i] == '$' && t.start[*i + 1] == '\'');
	a.s = t.start;
	a.len = t.len;
	a.i = *i + 2;
	a.dst = parena_alloc((size_t)t.len + 1);
	a.n = 0;
	while (a.i < a.len && a.s[a.i] != '\'')
	{
		if (a.s[a.i] == '\\' && a.i + 1 < a.len)
			ansic_escape(&a);
		else
			a.dst[a.n++] = a.s[a.i++];
	}
	a.i += (a.i < a.len);
	a.dst[a.n] = '\0';
	*i = a.i;
	push_ansic_node(ret, a.dst, a.n);
}
