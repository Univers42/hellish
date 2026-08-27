/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   reparse_all.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/27 12:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/27 12:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "decomposer.h"

/* The reparse pipeline, in one place because it has to run in one order and
   there is more than one way into it.
**
** ORDER IS LOAD-BEARING. reparse_subscript_assigns runs FIRST, while a word
** matching NAME[...]=value is still a single token: reparse_words splits on
** '$', '[' and '=', so after it has run there is no whole word left for the
** assignment classifier to recognise -- it sees the fragment `M[` and gives
** up. A literal subscript (M[fixed]=v) survives the wrong order only because
** it contains no '$' to split on, which is the literal-vs-variable asymmetry
** reported in issue #71.
**
** WHY IT IS A FUNCTION. There are two entry points into the shell's parser
** and they had drifted: parse_tokens() (used by -c, by a script argument and
** by piped stdin) ran all three passes, while run_parsed() -- the exec_string
** path used by `source`, `.`, `eval`, ~/.hellishrc, /etc/profile and
** PROMPT_COMMAND -- ran only the last two. So `M[$k]=v` was an assignment
** when you typed it and a command when you sourced it:
**
**     hellish: line 1: M[k]=VAL: command not found
**
** That is the hard blocker for any config framework, because every registry
** is an associative array written from a helper function and every rc file is
** sourced. Both callers now go through here, so a fourth pass cannot be added
** to one pipeline and forgotten in the other. See
** tests/source_subscript_test.py, which asserts this wiring and not just the
** behaviour. */
void	reparse_all(t_ast_node *node)
{
	reparse_subscript_assigns(node);
	reparse_words(node);
	reparse_assignment_words(node);
}
