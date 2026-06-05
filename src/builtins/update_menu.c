/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_menu.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update_menu.h"
#include <unistd.h>

/* Put the terminal in cbreak mode (no echo, byte-at-a-time) so we can read
   arrow keys. Saves the old settings into `saved`. 1 on success. */
static int	raw_on(struct termios *saved)
{
	struct termios	raw;

	if (tcgetattr(STDIN_FILENO, saved) != 0)
		return (0);
	raw = *saved;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
		return (0);
	return (1);
}

/* Read one key press, decoding arrow escapes and the vim-style j/k. An arrow
   arrives as the 3-byte burst ESC '[' 'A'|'B'; a lone ESC (or q / Ctrl-C / EOF)
   cancels. */
static int	read_key(void)
{
	unsigned char	b[4];
	ssize_t			r;

	r = read(STDIN_FILENO, b, sizeof(b));
	if (r <= 0 || b[0] == 3 || b[0] == 'q')
		return (K_ESC);
	if (b[0] == '\r' || b[0] == '\n')
		return (K_ENTER);
	if (r >= 3 && b[0] == 27 && b[1] == '[' && b[2] == 'A')
		return (K_UP);
	if (r >= 3 && b[0] == 27 && b[1] == '[' && b[2] == 'B')
		return (K_DOWN);
	if (b[0] == 27)
		return (K_ESC);
	if (b[0] == 'k')
		return (K_UP);
	if (b[0] == 'j')
		return (K_DOWN);
	return (K_NONE);
}

/* Apply an arrow key to the selection index, wrapping around the ends. */
static int	move_sel(int sel, int count, int key)
{
	if (key == K_UP)
		return ((sel + count - 1) % count);
	if (key == K_DOWN)
		return ((sel + 1) % count);
	return (sel);
}

/* Drive the chooser: draw the rows, then move the selection with the arrows and
   confirm with Enter, redrawing in place each time. Esc returns -1. */
int	run_menu(t_choice *ch, int count, int sel)
{
	struct termios	saved;
	int				key;

	if (count <= 0 || !raw_on(&saved))
		return (sel);
	menu_header();
	draw_options(ch, count, sel);
	key = K_NONE;
	while (key != K_ENTER)
	{
		key = read_key();
		if (key == K_ESC)
			sel = -1;
		sel = move_sel(sel, count, key);
		if (key == K_ESC)
			break ;
		ft_eprintf("\033[%dA", count);
		draw_options(ch, count, sel);
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &saved);
	ft_eprintf("\n");
	return (sel);
}
