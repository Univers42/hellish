/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vi_mode.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Readline vi editing mode.  The show-mode-in-prompt variable makes
   readline prefix the prompt with "(ins)" or "(cmd)" in vi mode so the
   user always knows which sub-mode they are in -- a quality-of-life
   touch that bash also enables by default in vi mode. */

#include <readline/readline.h>

/* Switch to vi key bindings (rl_editing_mode=0).  The mode indicator
   appears in the prompt automatically once show-mode-in-prompt is on. */
void	setup_vi_mode(void)
{
	rl_editing_mode = 0;
	rl_variable_bind("editing-mode", "vi");
	rl_variable_bind("keymap", "vi");
	rl_variable_bind("show-mode-in-prompt", "on");
}
