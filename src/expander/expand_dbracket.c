/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_dbracket.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+         */
/*   Created: 2026/06/10 06:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/10 06:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* True when `node` is a single, unquoted TT_WORD token whose raw text is
   exactly `lit`.  Used to recognise the [[ command word and the ==/=/!=
   operators STRUCTURALLY (on the raw token, before expansion) — bash
   parses the conditional grammar before any expansion, so an operator
   that only appears after expanding $op is not an operator. */
static bool	raw_word_is(t_ast_node *node, const char *lit)
{
	t_token	*t;
	int		len;

	if (node->node_type != AST_WORD || node->children.len != 1
		|| ((t_ast_node *)node->children.ctx)[0].node_type != AST_TOKEN)
		return (false);
	t = &((t_ast_node *)node->children.ctx)[0].token;
	if (t->tt != TT_WORD)
		return (false);
	len = ft_strlen(lit);
	if (t->len != len)
		return (false);
	return (ft_strncmp(t->start, lit, len) == 0);
}

/* Is this simple command a [[ ... ]] conditional?  (First child is the
   raw word `[[`.) */
bool	db_head_detect(t_ast_node *node)
{
	if (node->children.len == 0)
		return (false);
	return (raw_word_is((t_ast_node *)node->children.ctx, "[["));
}

/* Expand one WORD of a [[ ... ]] conditional.  POSIX/bash: no field
   splitting and no pathname expansion ever happen inside [[ — and the
   word after an unquoted ==, = or != is a PATTERN whose quoted
   metacharacters match literally.  Both rules are exactly the case/esac
   rules, so the case machinery is reused verbatim: expand_case_word for
   ordinary operands, expand_case_pattern for the pattern slot.  The
   resulting strings are plain heap (word_free routes them to xfree, the
   same way alias-rebuilt argv words are handled). */
int	process_db_child(t_shell *state, t_expander_simple_cmd *exp,
		t_executable_cmd *ret)
{
	char	*s;

	if (exp->pat_next)
		s = expand_case_pattern(state, exp->curr);
	else
		s = expand_case_word(state, exp->curr);
	if (exp->pat_next)
		exp->pat_next = false;
	else
		exp->pat_next = (raw_word_is(exp->curr, "==")
				|| raw_word_is(exp->curr, "=")
				|| raw_word_is(exp->curr, "!="));
	if (!s)
		return (1);
	vec_push(&ret->argv, &s);
	exp->found_first = true;
	return (0);
}
