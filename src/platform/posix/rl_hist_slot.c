/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_hist_slot.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"
#include "rl_private.h"

/* The four arrow encodings, in two keymaps, make eight slots: bit
** (map * 4 + key). key bit 0 picks Down over Up, key bit 1 picks the
** application-cursor "\033O" form over "\033[". A slot is "ours" while
** hist_bits has its bit; it is "the user's" once a bind / bindkey / inputrc
** line has set it, and then never touched again (hist_user). */

/* Write slot `bit`'s key sequence into `key` (4 bytes). */
static void	hp_key(int bit, char *key)
{
	key[0] = '\033';
	key[1] = '[';
	if (bit & 2)
		key[1] = 'O';
	key[2] = 'A';
	if (bit & 1)
		key[2] = 'B';
	key[3] = '\0';
}

/* The stock function of the slot, and the history-search one that
   replaces it. */
static void	hp_funcs(int bit, rl_command_func_t **stock,
	rl_command_func_t **srch)
{
	*stock = rl_get_previous_history;
	*srch = rl_history_search_backward;
	if (bit & 1)
	{
		*stock = rl_get_next_history;
		*srch = rl_history_search_forward;
	}
}

/* A marker no user can bind: a slot still holding it after the user's
   bind lines ran was not touched by them. */
static int	hp_mark(int count, int key)
{
	(void)count;
	(void)key;
	return (0);
}

/* Before the user's bindings are replayed: park every slot that is the
   stock function (or our search) on the marker. */
void	rl_hist_arm(t_rl *l, int bit)
{
	char				key[4];
	Keymap				map;
	rl_command_func_t	*stock;
	rl_command_func_t	*srch;
	rl_command_func_t	*cur;

	hp_key(bit, key);
	hp_funcs(bit, &stock, &srch);
	map = emacs_standard_keymap;
	if (bit >= 4)
		map = vi_insertion_keymap;
	cur = rl_function_of_keyseq(key, map, NULL);
	if (l->hist_user >> bit & 1)
		return ;
	if (cur == stock || (cur == srch && (l->hist_bits >> bit & 1)))
		rl_bind_keyseq_in_map(key, hp_mark, map);
}

/* After the replay: a slot still on the marker gets the wanted function
   (search when on, stock when off); any other function there is the
   user's own choice, which is kept for good. */
void	rl_hist_settle(t_rl *l, int bit)
{
	char				key[4];
	Keymap				map;
	rl_command_func_t	*stock;
	rl_command_func_t	*srch;
	rl_command_func_t	*cur;

	hp_key(bit, key);
	hp_funcs(bit, &stock, &srch);
	map = emacs_standard_keymap;
	if (bit >= 4)
		map = vi_insertion_keymap;
	cur = rl_function_of_keyseq(key, map, NULL);
	if (l->hist_user >> bit & 1)
		return ;
	l->hist_bits &= ~(1u << bit);
	if (cur != hp_mark)
		l->hist_user |= 1u << bit;
	else if (l->hist_want)
		l->hist_bits |= 1u << bit;
	if (cur == hp_mark && l->hist_want)
		rl_bind_keyseq_in_map(key, srch, map);
	else if (cur == hp_mark)
		rl_bind_keyseq_in_map(key, stock, map);
}
