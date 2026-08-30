/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   procsub_assign.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* A process substitution on the RIGHT-HAND SIDE of an assignment.
**
**     x=<(echo hi)          bash: x holds /dev/fd/63
**     x==(:)                zsh:  x holds /tmp/zshXXXXXX   (#83)
**     f() { local t==(:); } zsh:  t holds the same
**
** The lexer ends the word at the operator, so the parser sees TWO siblings:
** an assignment word `x=` and an AST_PROC_SUB.  Nothing joined them, so the
** path became its own argv entry -- which the executor then tried to RUN
** ("/dev/fd/3: No such file or directory") while x was left empty.  Inside
** `local` there was not even a message: the path was simply a second
** operand that local ignored.
**
** Filed against zsh's `=(cmd)`, but the bug is neither zsh-specific nor
** =()-specific: plain bash `x=<(cmd)` did the same thing, and no golden case
** covered it.
*/

/* Is this process substitution the VALUE of the word before it?  The
   parser answered the adjacency question while the tokens were still
   offsets into one buffer (see glued_to_previous in parse_simple_cmd.c);
   here we only need it to continue an ASSIGNMENT, not any old word.

     x=<(cmd)         glued, previous ends in `=`  -> the value
     echo pre<(cmd)   glued, but a plain word      -> left alone

   The second is also wrong today (bash concatenates; we emit two fields)
   but it is a different gap, and half-fixing it here would make the words
   inconsistent with the trailing `<(cmd)post` case, which nothing handles
   yet. */
bool	procsub_is_assign_rhs(t_expander_simple_cmd *exp)
{
	t_ast_node	*prev;

	if (!exp->curr->glued || exp->i == 0 || !exp->node)
		return (false);
	prev = (t_ast_node *)vec_idx(&exp->node->children, exp->i - 1);
	return (prev->node_type == AST_ASSIGNMENT_WORD);
}

/* `a + b` as a fresh string, releasing `a`.  NULL reads as empty so an
   assignment with no literal part (`x=<(cmd)`) needs no special case. */
static char	*join_free(char *a, const char *b)
{
	char	*out;

	if (!a)
		return (ft_strdup(b));
	out = ft_strjoin(a, b);
	word_free(a);
	return (out);
}

/* Append `path` to whichever destination the assignment before us went to:
   pre_assigns while the command word is still unseen (`x=<(cmd) prog`), the
   last argv entry once it has been seen (`local t=<(cmd)`).  That mirrors
   expand_simple_cmd_assignment's own split, so the two cannot disagree.
     word_free is right for both: it routes a slab pointer to the slab and
   an xmalloc'd one to xfree, and argv holds a mix of the two. */
void	procsub_join_assign(t_expander_simple_cmd *exp, t_executable_cmd *ret,
			char *path)
{
	t_env	*ev;
	char	**slot;

	if (!exp->found_first && ret->pre_assigns.len)
	{
		ev = (t_env *)vec_idx(&ret->pre_assigns, ret->pre_assigns.len - 1);
		ev->value = join_free(ev->value, path);
	}
	else if (ret->argv.len)
	{
		slot = (char **)vec_idx(&ret->argv, ret->argv.len - 1);
		*slot = join_free(*slot, path);
	}
	xfree(path);
}
