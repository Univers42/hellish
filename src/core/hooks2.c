/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   hooks2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 13:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 13:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "helpers.h"
#include "sh_input.h"

/* The preexec side: fired once per typed line, just before it runs, with
** the line itself as $1.
**
** The line is copied out of state->input by LENGTH rather than read as a C
** string. The input buffer is a t_string the reader appends into; treating
** it as NUL-terminated is true today and is exactly the kind of assumption
** that becomes false in a later refactor with no visible symptom until a
** hook receives trailing garbage.
**
** The trailing newline goes, because $1 is the command the user typed and
** a hook that echoes it should not produce a blank line.
*/
void	run_preexec(t_shell *state)
{
	char	*line;
	size_t	n;

	if (state->metinp != INP_RL || !state->input.ctx || !state->input.len)
		return ;
	n = state->input.len;
	while (n > 0 && (((char *)state->input.ctx)[n - 1] == '\n'
		|| ((char *)state->input.ctx)[n - 1] == '\0'))
		n--;
	if (n == 0)
		return ;
	line = ft_strndup((char *)state->input.ctx, n);
	if (!line)
		return ;
	run_hook_funcs(state, "HELLISH_PREEXEC_FUNCS", line);
	xfree(line);
}
