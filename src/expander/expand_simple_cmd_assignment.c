/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_simple_cmd_assignment.c                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 19:29:14 by marvin            #+#    #+#             */
/*   Updated: 2026/01/22 19:29:14 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "lexer.h"
#include "sys.h"
#include "ft_builtins.h"

/* Perform assignment -> word conversion and expand into argv.  Operates on
   a private clone so the caller's node stays intact (loops reuse it).
   Returns 1 on unwind request, 0 otherwise.

   `export` gets EW_KEEP_AS_ONE because an assignment word does not field
   split -- `export PATH=$PATH` is one operand however many spaces $PATH
   holds.  It used to also get the RAW SOURCE TEXT restored over any argv
   entry ending in `=`, which paired with a second quote-strip-and-expand
   inside the export builtin: two wrongs that cancelled on `export s=$UNSET`
   and destroyed everything else (`export s=''` stored two literal quotes,
   `export s='x$y'` stored `x`).  Both halves are gone; the ordinary
   expansion is the answer, as it already was for declare/readonly/local. */
static int	expand_assignment_word_and_fixup(t_shell *state,
					t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	t_ast_node	scratch;
	int			flags;

	scratch = clone_ast(exp->curr);
	assignment_word_to_word(&scratch);
	flags = EW_NO_GLOB;
	if (exp->export)
		flags |= EW_KEEP_AS_ONE;
	expand_word_glob_ctl(state, &scratch, &ret->argv, flags);
	if (get_g_sig()->should_unwind)
		return (1);
	return (0);
}

/* Expand one AST_ASSIGNMENT_WORD child of a simple command.  Before the
   first real command word (exp->found_first == false), the assignment is
   staged in pre_assigns — it may become a temporary env var for the command.
   POSIX wants the assignments applied left to right, each RHS seeing the
   earlier ones (`x=5 y=$x` gives y=5), so the already-staged ones are
   temporarily applied around the RHS expansion and rolled back — argv
   words expanded later must still see the original environment.
   After the command name is known, the assignment becomes an argv word
   (e.g. `export VAR=val`) and is expanded with no glob (EW_NO_GLOB). */
int	expand_simple_cmd_assignment(t_shell *state,
		t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	t_env		tmp;
	t_vec		saves;

	if (!exp->found_first)
	{
		saves = apply_temp_assigns(state, &ret->pre_assigns);
		tmp = assignment_to_env(state, exp->curr);
		restore_temp_assigns(state, &saves);
		vec_push(&ret->pre_assigns, &tmp);
		return (0);
	}
	if (expand_assignment_word_and_fixup(state, exp, ret))
		return (1);
	return (0);
}
