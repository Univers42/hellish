/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_func_anon.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"

/* Run zsh's anonymous function -- `() { ... }`, defined and called once.
**
** It reuses execute_func_call rather than repeating its ten lines of frame,
** scope and positional-parameter bookkeeping. That is the whole point: an
** anonymous function IS a function, and the two must not drift on what a
** call frame does -- `local`, `return`, the RETURN trap, the dialect swap
** and $0 all have to behave identically or the construct is a lookalike
** rather than the thing.
**
** The t_shell_func is a STACK value and is never stored. Registering it under
** a reserved name would have been the other way, and it brings two problems
** this does not have: a name a script could collide with, and nesting -- an
** inner anonymous function would overwrite the entry its caller is still
** running out of. One struct per activation is what makes nesting free.
**
** `zsh` is true because the construct only parses in the zsh dialect, so a
** body that runs here was written in it. Nothing here is freed and nothing
** leaks: frame_push copies both strings, the name is a literal, and src is
** borrowed from the frame stack rather than the duplicate store_function
** takes -- there is no owner to hand it back to.
*/
t_execution_state	execute_anon_func(t_shell *state, t_executable_node *exe)
{
	t_execution_state	st;
	t_shell_func		anon;
	t_vec				argv;
	char				*self;

	if (!exe->node->children.len)
		return (res_status(0));
	anon.name = "(anonymous)";
	anon.src = (char *)frame_src_name(state);
	anon.text = NULL;
	anon.zsh = true;
	anon.body = *(t_ast_node *)vec_idx(&exe->node->children, 0);
	vec_init(&argv);
	argv.elem_size = sizeof(char *);
	self = anon.name;
	vec_push(&argv, &self);
	st = execute_func_call(state, &anon, &argv);
	return (xfree(argv.ctx), st);
}
