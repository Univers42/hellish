/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_menu2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update_menu.h"

/* The chooser's question line, with a key hint and a salmon prompt mark. */
void	menu_header(void)
{
	const char	*hint;

	hint = "(up/down, Enter, Esc)";
	if (header_use_unicode())
		hint = "(\xe2\x86\x91/\xe2\x86\x93 \xc2\xb7 Enter \xc2\xb7 Esc)";
	ft_eprintf("\n\033[1;38;5;209m?\033[0m \033[1mHow should hellish "
		"update?\033[0m  \033[38;5;245m%s\033[0m\n\n", hint);
}

/* Draw one option row: a salmon arrow + bright label when selected (else a
   muted label), then the grey command and a tag for the detected method. */
static void	draw_row(t_choice *c, int active)
{
	const char	*cur;

	cur = ">";
	if (header_use_unicode())
		cur = "\xe2\x9d\xaf";
	ft_eprintf("\r\033[K");
	if (active)
		ft_eprintf("  \033[1;38;5;209m%s %-18s\033[0m",
			cur, c->label);
	else
		ft_eprintf("    \033[38;5;250m%-18s\033[0m", c->label);
	ft_eprintf(" \033[38;5;245m%s\033[0m", c->cmd);
	if (c->detected)
		ft_eprintf("  \033[38;5;78m(detected)\033[0m");
	ft_eprintf("\n");
}

/* (Re)draw all option rows, highlighting the one at `sel`. */
void	draw_options(t_choice *ch, int count, int sel)
{
	int	i;

	i = -1;
	while (++i < count)
		draw_row(&ch[i], i == sel);
}
