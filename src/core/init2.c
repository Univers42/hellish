/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init2.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

/* stdin is not a tty (piped input: `echo cmd | hellish`, or a redirected
   file). We switch to INP_NOTTY so the REPL skips readline prompts and
   reads raw input directly from fd 0. should_update_ctx signals the lexer
   that context info may change between reads. */
void	init_stdin_notty(t_shell *state)
{
	state->metinp = INP_NOTTY;
	state->rl.should_update_ctx = true;
}
