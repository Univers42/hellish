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

/* The dialect, for the duration of one reparse pass.
**
** The reparser is the one part of the pipeline that does NOT carry a
** t_shell: every function in it takes a t_reparser, and threading state
** through eight functions in the hottest path in the shell to answer one
** question is a poor trade. reparse_all is the single entry to the whole
** pipeline -- that is why it exists -- so the dialect is a parameter OF the
** pass, stashed where the pass can reach it.
**
** Saved and restored rather than merely set, because the pipeline re-enters
** itself: a command substitution inside a word parses through here again.
** The value is the same either way (it comes from the same t_shell), but a
** latch that unwinds cannot be the thing that is wrong later. */
static bool	g_reparse_zsh = false;

bool	reparse_zsh(void)
{
	return (g_reparse_zsh);
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
	bool	was;

	was = g_reparse_zsh;
	g_reparse_zsh = zsh_mode(state);
	reparse_subscript_assigns(node);
	reparse_words(node);
	reparse_assignment_words(node);
	g_reparse_zsh = was;
}
