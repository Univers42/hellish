/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_word2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:35 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:35 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "reparser_private.h"
#include "ft_glob.h"

/* Entry point: take a raw TT_WORD/TT_ASSIGN_WORD token from the parser
   and decompose it into a tree of typed subtokens (TT_SQWORD, TT_DQWORD,
   TT_ENVVAR, TT_WORD ...). The returned node owns its children vec; the
   caller must free_ast() it when done. This is the "second parse pass" that
   turns the flat lexer output into something the expander can act on. */
t_ast_node	reparse_word(t_token t, bool no_squote)
{
	t_ast_node	ret;
	t_reparser	rp;

	ret = create_node_type(AST_WORD);
	vec_init(&ret.children);
	ret.children.elem_size = sizeof(t_ast_node);
	create_reparser(&rp, ret, t, &(int){0});
	rp.no_squote = no_squote;
	loop_node_rp(&rp);
	ret = rp.current_node;
	return (ret);
}

/* An extglob group is ONE lexical unit and must survive the reparse whole.
   Without this the group is torn apart on its own parentheses -- `@(a*)`
   became the four subtokens `@`, `(`, `a*`, `)` -- and word_to_glob then
   glob-tokenized each fragment on its own, so the pattern the matcher
   finally saw was never the pattern that was written.  `@(aa|ab)` escaped
   that only because it happens to contain nothing the reparser splits on,
   which is why the gap looked like "some extglob patterns work".
     The group is emitted verbatim, so a `$var` inside one is not expanded
   (v1 limitation: `@(a|b)` and `@(*.c|*.h)` are the shapes that matter, and
   an unexpanded group is at worst a pattern that does not match, never a
   wrong match). Returns 0 when there is no group here, or when the group
   runs past the end of this token. */
int	reparse_extglob(t_ast_node *ret, int *i, t_token t)
{
	int	n;

	n = extglob_ahead(t.start + *i);
	if (n <= 0 || *i + n > t.len)
		return (0);
	push_subtoken_node(ret, t, create_interval(*i, *i + n), TT_WORD);
	*i += n;
	return (1);
}
