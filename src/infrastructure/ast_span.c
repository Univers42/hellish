/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast_span.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 14:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 14:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ast.h"
#include "shell.h"

/* Recover the SOURCE TEXT a subtree was parsed from.
**
** This is what makes `declare -f` possible (issue #71 item 2). The obvious
** alternative was a deparser -- walk the 23 t_ast_type cases and re-emit
** shell syntax -- and it is the wrong tool here. A deparser has to
** re-invent quoting, redirection order, here-doc bodies, case patterns and
** operator spacing, and every one of those is a chance to print something
** that no longer means what the user wrote. For a feature whose entire job
** is "show me what this plugin actually defined", a subtly wrong answer is
** worse than none -- the same objection issue #71 item 4 makes about knobs
** that report success and do nothing.
**
** The exact text is already reachable. Every token the LEXER produced is a
** slice into the input buffer; only the expander and deep_clone_ast ever set
** token.allocated (see incs/public/token.h, and exec_lineno.c:25 which tests
** the same property to map a token back to a line). So between parse and
** expansion, min(start) .. max(start+len) over a subtree IS its source.
**
** TIMING IS LOAD-BEARING: this must run on the node execute_func_def was
** handed, BEFORE store_function's deep_clone_ast, which copies every token
** onto the heap and sets allocated -- after that the span is gone.
**
** Divergence from bash, deliberate and documented: bash re-indents what it
** prints, so `f() { echo hi; }` comes back as a four-line pretty-printed
** form. This returns what was actually written. It round-trips through eval
** either way, and for reading someone else's plugin the original layout is
** the more useful answer. */

/* Widen [lo, hi) to cover every token in the subtree that is still a slice
   of the input buffer. Heap-copied tokens are skipped: they belong to a
   different allocation, and comparing pointers across allocations would be
   meaningless even where it is not undefined. */
static void	span_walk(t_ast_node *node, const char **lo, const char **hi)
{
	t_ast_node	*kids;
	size_t		i;

	if (node->token.start && !node->token.allocated && node->token.len > 0)
	{
		if (!*lo || node->token.start < *lo)
			*lo = node->token.start;
		if (!*hi || node->token.start + node->token.len > *hi)
			*hi = node->token.start + node->token.len;
	}
	kids = (t_ast_node *)node->children.ctx;
	i = 0;
	while (i < node->children.len)
		span_walk(&kids[i++], lo, hi);
}

/* The source text of `node`, freshly allocated, or NULL when the subtree
   holds no in-buffer token (an eval'd or already-expanded body). */
char	*ast_source_text(t_ast_node *node)
{
	const char	*lo;
	const char	*hi;

	lo = NULL;
	hi = NULL;
	span_walk(node, &lo, &hi);
	if (!lo || !hi || hi <= lo)
		return (NULL);
	if ((size_t)(hi - lo) > AST_SPAN_MAX)
		return (NULL);
	return (ft_strndup((char *)lo, (size_t)(hi - lo)));
}
