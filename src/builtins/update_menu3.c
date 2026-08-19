/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_menu3.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update_menu.h"
#include <unistd.h>

/* Fill choice `i` for origin `o` (its label + the exact upgrade command). */
static int	add_choice(t_choice *ch, int i, t_origin o, const char *repo)
{
	ch[i].origin = o;
	ch[i].label = origin_label(o);
	origin_command(o, repo, ch[i].cmd, sizeof(ch[i].cmd));
	ch[i].detected = 0;
	return (i + 1);
}

/* Build the menu: the detected method first (marked, pre-selected), then the
   other path-free methods. A source checkout only appears when detected, since
   the others are the only ones that upgrade without knowing a clone path. */
static int	build_choices(t_choice *ch, t_origin det, const char *repo)
{
	t_origin	pool[4];
	int			i;
	int			k;

	i = add_choice(ch, 0, det, repo);
	ch[0].detected = 1;
	pool[0] = ORIGIN_NPM;
	pool[1] = ORIGIN_PNPM;
	pool[2] = ORIGIN_DOCKER;
	pool[3] = ORIGIN_BINARY;
	k = -1;
	while (++k < 4)
	{
		if (pool[k] != det)
			i = add_choice(ch, i, pool[k], "");
	}
	return (i);
}

/* `update --now`: on a TTY, always present the chooser (pre-selected on the
   detected method) and run whatever the user picks; off a TTY, run the detected
   method directly so scripts never block. */
int	update_interactive(t_origin det, const char *repo, const char *tag)
{
	t_choice	ch[5];
	const char	*r;
	int			count;
	int			sel;

	if (!isatty(STDIN_FILENO) || !isatty(STDERR_FILENO))
		return (run_origin_update(det, repo, tag));
	count = build_choices(ch, det, repo);
	sel = run_menu(ch, count, 0);
	if (sel < 0)
	{
		ft_eprintf("hellish: update cancelled\n");
		return (1);
	}
	r = "";
	if (ch[sel].origin == ORIGIN_SOURCE)
		r = repo;
	ft_eprintf("hellish: %s\n", ch[sel].cmd);
	return (run_origin_update(ch[sel].origin, r, tag));
}
