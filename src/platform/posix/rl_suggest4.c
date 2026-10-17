/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest4.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "zle.h"

/* Taking the ghost down, and forgetting it. */

/* Wipe the ghost from the screen without touching the text: from the
   column it was painted at, to the end of the row, with the cursor put
   back after. Used when the line is about to be accepted or the read
   interrupted -- neither of which gives the painter another frame -- so
   that no ghost ends up in the scrollback. */
void	rl_suggest_unpaint(void)
{
	t_shell		**cell;
	t_string	f;

	cell = zle_state_cell();
	if (!*cell || !(*cell)->rl.sg_painted)
		return ;
	(*cell)->rl.sg_painted = false;
	vec_init(&f);
	f.elem_size = 1;
	vec_push_str(&f, "\0337");
	if (rl_push_col(&f, (*cell)->rl.sg_col + 1))
	{
		vec_push_str(&f, "\033[K\0338");
		rl_out_write(f.ctx, f.len);
	}
	xfree(f.ctx);
}

/* A resize wiped the screen under the line: nothing of the ghost is
   there any more, and the next frame paints it afresh (or not). */
void	rl_suggest_stale(void)
{
	t_shell	**cell;

	cell = zle_state_cell();
	if (*cell)
		(*cell)->rl.sg_painted = false;
}

/* The read is over (or about to start): nothing of the ghost, of the
   search it was found with or of a widget's POSTDISPLAY outlives it. The
   history may have changed since, so the cache cannot either. */
void	rl_suggest_end(t_shell *state)
{
	t_rl	*l;

	l = &state->rl;
	xfree(l->sg_ent);
	xfree(l->sg_prev);
	xfree(l->sg_post);
	xfree(l->sg_post_at);
	xfree(l->sg_pub);
	l->sg_ent = NULL;
	l->sg_prev = NULL;
	l->sg_post = NULL;
	l->sg_post_at = NULL;
	l->sg_pub = NULL;
	l->sg_txt = NULL;
	l->sg_known = false;
	l->sg_painted = false;
}

/* Columns taken by everything left of the text on the prompt's row: the
   row readline was handed, and the mode string it puts in front of it
   (the "(ins)" of vi mode) when show-mode-in-prompt is on. */
int	rl_prompt_cols(t_rl *l)
{
	const char	*on;
	const char	*name;
	const char	*v;

	on = rl_variable_value("show-mode-in-prompt");
	if (!on || ft_strcmp(on, "on"))
		return (l->rp_prompt_w);
	name = "emacs-mode-string";
	if (rl_get_keymap() == vi_insertion_keymap)
		name = "vi-ins-mode-string";
	if (rl_get_keymap() == vi_movement_keymap)
		name = "vi-cmd-mode-string";
	v = rl_variable_value(name);
	if (!v)
		return (l->rp_prompt_w);
	return (l->rp_prompt_w + visible_width_cstr(v));
}
