/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_arith.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "arith.h"

char	*expand_arith_vars(t_shell *state, const char *s, int len);

/* True when the slice holds no expression at all (only blanks). */
static bool	xa_blank(t_token tok)
{
	int	i;

	i = 0;
	while (i < tok.len && (tok.start[i] == ' ' || tok.start[i] == '\t'
			|| tok.start[i] == '\n'))
		i++;
	return (i == tok.len);
}

/* Evaluate one arithmetic slice; a blank slice is a no-op that reports 1
** (the for-arith header treats a missing cond as "keep looping" and a
** missing init/step as nothing to do).  Errors surface through *err.
**
** THE `$` PASS IS WHY $(( )) AND (( )) AGREED ON SO LITTLE. $(( )) is
** reached through the expander, which resolves every $var and ${...} in the
** text before the arithmetic evaluator sees it. (( )) is a COMMAND and
** arrives here as raw source, so it only ever knew what the arithmetic
** lexer itself knows -- $name and ${name}, and nothing more:
**
**     A=(7 8); echo $(( ${#A[@]} ))     2        correct
**     A=(7 8); (( n = ${#A[@]} ))       n=0      silently
**     for ((i=0; i < ${#A[@]}; i++))    zero iterations, status 0
**
** A loop that runs no times and reports success is the worst shape this can
** take, and it is why git-completion's __git_main handed back an empty
** COMPREPLY after loading perfectly. Same text, same expander, one answer.
** The memchr keeps the common `(( i < n ))` off the new path entirely, so
** nothing that already worked pays for it.
*/
static long long	xa_eval(t_shell *state, t_token tok, bool *err)
{
	char		*txt;
	long long	v;

	if (xa_blank(tok))
		return (1);
	if (!ft_memchr(tok.start, '$', (size_t)tok.len))
		return (arith_eval(state, tok.start, tok.len, err));
	txt = expand_arith_vars(state, tok.start, tok.len);
	if (!txt)
		return (arith_eval(state, tok.start, tok.len, err));
	v = arith_eval(state, txt, (int)ft_strlen(txt), err);
	return (xfree(txt), v);
}

/* (( expr )): evaluate and map value!=0 to status 0, value==0 to 1, an
   evaluation error to 1 as well (bash prints the error and fails).  An
   empty or blank (( )) evaluates to 0, i.e. status 1 — xa_eval's blank
   rule is for the for-header, so blankness is re-checked here. */
t_execution_state	execute_arith_cmd(t_shell *state, t_executable_node *exe)
{
	bool		err;
	long long	v;

	err = false;
	v = xa_eval(state, exe->node->token, &err);
	if (xa_blank(exe->node->token))
		v = 0;
	if (err || v == 0)
		return (res_status(1));
	return (res_status(0));
}

/* for (( init; cond; step )) do body done — children are the three header
   slices then the body.  Same loop-control contract as execute_while:
   errexit is masked around the cond, handle_loop_ctl consumes one level
   of break/continue, and the loop's status is the last body status. */
/* One iteration: check the cond slice (errexit masked, like a while
   condition), run the body, honour break/continue, then run the step
   slice.  Returns false when the loop must stop. */
static bool	fa_iter(t_shell *state, t_executable_node *exe,
				t_execution_state *body, bool *err)
{
	t_executable_node	child;
	long long			v;

	state->errexit_off++;
	v = xa_eval(state,
			((t_ast_node *)vec_idx(&exe->node->children, 1))->token, err);
	state->errexit_off--;
	if (*err || v == 0)
		return (false);
	child = create_exe_node(STDIN_FILENO, STDOUT_FILENO,
			vec_idx(&exe->node->children, 3), true);
	*body = execute_tree_node(state, &child);
	if (handle_loop_ctl(state))
		return (false);
	xa_eval(state,
		((t_ast_node *)vec_idx(&exe->node->children, 2))->token, err);
	return (true);
}

t_execution_state	execute_for_arith(t_shell *state, t_executable_node *exe)
{
	t_execution_state	body_status;
	bool				err;

	ft_assert(exe->node->children.len == 4);
	err = false;
	body_status = res_status(0);
	xa_eval(state,
		((t_ast_node *)vec_idx(&exe->node->children, 0))->token, &err);
	state->loop_depth++;
	while (!err && fa_iter(state, exe, &body_status, &err))
		;
	state->loop_depth--;
	return (body_status);
}
