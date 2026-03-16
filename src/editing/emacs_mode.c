/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   emacs_mode.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <readline/readline.h>

void	setup_emacs_mode(void)
{
	rl_editing_mode = 1;
	rl_variable_bind("editing-mode", "emacs");
	rl_variable_bind("keymap", "emacs");
}
