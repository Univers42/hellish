/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_menu.h                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UPDATE_MENU_H
# define UPDATE_MENU_H

# include "update.h"
# include "header.h"
# include <termios.h>

/* Keys the chooser understands, decoded from raw terminal input. */
# define K_NONE  0
# define K_UP    1
# define K_DOWN  2
# define K_ENTER 3
# define K_ESC   4

/* One selectable upgrade method: its origin, a short label, the exact shell
   command it would run, and whether it is the auto-detected one. */
typedef struct s_choice
{
	t_origin	origin;
	const char	*label;
	char		cmd[256];
	int			detected;
}	t_choice;

/* Print the chooser's question + key hint (once, above the option rows). */
void	menu_header(void);

/* (Re)draw the `count` option rows, highlighting row `sel`. */
void	draw_options(t_choice *ch, int count, int sel);

/* Run the arrow-key chooser starting on row `start`; returns the chosen index,
   or -1 if cancelled (Esc / Ctrl-C / EOF). Falls back to `start` without a TTY.
*/
int		run_menu(t_choice *ch, int count, int start);

/* `update --now`: build the method list, let the user pick (always, when on a
   TTY), and run the chosen upgrade. Without a TTY, runs the detected method. */
int		update_interactive(t_origin detected, const char *repo,
			const char *tag);

#endif
