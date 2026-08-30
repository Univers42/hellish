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
#include "shell.h"
#include "ft_glob.h"

/* The dialect, for the reparser.
**
** The reparser is the one part of the pipeline that does NOT carry a
** t_shell: every function in it takes a t_reparser, and threading state
** through eight functions in the hottest path in the shell to answer one
** question is a poor trade.  So it reads the same process-wide cell the
** glob layer does, which zsh_mode_swap keeps in step with t_shell.setopt.
**
** This USED to be a latch armed only around reparse_all, and that was a
** genuine bug rather than a simplification: reparse_all is the PARSE-time
** entry, and words are reparsed again at EXPANSION time -- the word half of
** `${x:-word}`, an array subscript -- through expand_param_word, which does
** not go through reparse_all.  The latch read false there, so `a[$#a]=()`
** saw `$#a` split into the positional count and a stray name and evaluated
** the subscript of a pop as `0a`. Reading the live cell is both simpler and
** the only version that answers the same way at both times. */
bool	reparse_zsh(void)
{
	return (glob_zsh() != 0);
}

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
void	reparse_all(t_shell *state, t_ast_node *node)
{
	(void)state;
	reparse_subscript_assigns(node);
	reparse_words(node);
	reparse_assignment_words(node);
}
