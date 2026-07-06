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

/* Grab the original (pre-expansion) full token for an assignment word so it
   can be restored into argv after expansion.  This is needed because `export
   f(x)=val` and similar compound-assignment forms expand into "f(x)=" which
   must be replaced by the original text in the argv display. */
static t_token_old	capture_original_full_token(t_ast_node *node)
{
	t_token_old	full;
	t_ast_node	*orig;

	full = init_token_old();
	if (!node)
		return (full);
	if (node->node_type == AST_ASSIGNMENT_WORD && node->children.len > 1)
	{
		orig = &((t_ast_node *)node->children.ctx)[1];
		if (orig->node_type == AST_WORD)
			full = get_old_token(*orig);
	}
	else if (node->node_type == AST_WORD)
		full = get_old_token(*node);
	return (full);
}

/* After expansion, argv entries that end with '=' come from assignment words
   that field-split to "NAME=".  Replace them with a dup of the full original
   token so export/typeset display the full assignment text, not just "NAME=".
   Only present is checked — zero-length or NULL full.start entries skip. */
static void	replace_argv_entries_with_full_token(t_vec *argv,
				t_token_old full)
{
	size_t	ai;
	char	*s;
	char	*orig;

	if (!argv || !argv->ctx || !full.present)
		return ;
	ai = 0;
	while (ai < argv->len)
	{
		s = ((char **)argv->ctx)[ai];
		if (s && s[0] && s[ft_strlen(s) - 1] == EQ)
		{
			orig = ft_strndup(full.start, full.len);
			xfree(((char **)argv->ctx)[ai]);
			((char **)argv->ctx)[ai] = orig;
		}
		ai++;
	}
}

/* Perform assignment -> word conversion, expand into argv and apply fixup.
   Operates on a private clone so the caller's node stays intact (loops reuse
   it). Returns 1 on unwind request, 0 otherwise.
   The full-token restore only applies in export/declaration context: for a
   plain argv word like `echo arg=$UNSET`, "arg=" IS the correct expansion
   (bash prints it), and restoring the raw text would resurrect the unset
   `$UNSET` reference the expansion just (correctly) emptied out. */
static int	expand_assignment_word_and_fixup(t_shell *state,
					t_expander_simple_cmd *exp, t_executable_cmd *ret)
{
	t_token_old	full;
	t_ast_node	scratch;
	int			flags;

	full = capture_original_full_token(exp->curr);
	scratch = clone_ast(exp->curr);
	assignment_word_to_word(&scratch);
	flags = EW_NO_GLOB;
	if (exp->export)
		flags |= EW_KEEP_AS_ONE;
	expand_word_glob_ctl(state, &scratch, &ret->argv, flags);
	if (get_g_sig()->should_unwind)
		return (1);
	if (exp->export)
		replace_argv_entries_with_full_token(&ret->argv, full);
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
