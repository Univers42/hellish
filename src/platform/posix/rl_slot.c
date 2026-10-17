/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_slot.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* Keys that hold one of readline's stock functions until a feature wants
** them: Up / Down for history search (HELLISH_HISTORY_PREFIX) and Right /
** End / ^F for accepting the inline suggestion (HELLISH_AUTOSUGGEST).
** A slot is "ours" while slot_bits has its bit; it is "the user's" once a
** bind / bindkey / inputrc line has set it, and then it is never touched
** again (slot_user). The arrows come in two encodings (CSI and the
** application-cursor SS3 form) and two keymaps, hence the rows. */

/* A marker no user can bind: a slot still holding it after the user's
   bind lines ran was not touched by them. */
static int	rl_slot_mark(int count, int key)
{
	(void)count;
	(void)key;
	return (0);
}

/* Before the user's bindings are replayed: park every slot that is the
   stock function (or our replacement) on the marker. */
void	rl_slot_arm(t_rl *l, int i)
{
	t_slot				s;
	Keymap				map;
	rl_command_func_t	*stock;
	rl_command_func_t	*repl;
	rl_command_func_t	*cur;

	s = rl_slot_at(i);
	rl_slot_funcs(&s, &stock, &repl);
	map = emacs_standard_keymap;
	if (s.vi)
		map = vi_insertion_keymap;
	cur = rl_function_of_keyseq(s.key, map, NULL);
	if (l->slot_user >> i & 1)
		return ;
	if (cur == stock || (cur == repl && (l->slot_bits >> i & 1)))
		rl_bind_keyseq_in_map(s.key, rl_slot_mark, map);
}

/* After the replay: a slot still on the marker gets the wanted function
   (the replacement when its feature is on, stock when off); any other
   function there is the user's own choice, which is kept for good. */
void	rl_slot_settle(t_rl *l, int i)
{
	t_slot				s;
	Keymap				map;
	rl_command_func_t	*stock;
	rl_command_func_t	*repl;
	rl_command_func_t	*cur;

	s = rl_slot_at(i);
	rl_slot_funcs(&s, &stock, &repl);
	map = emacs_standard_keymap;
	if (s.vi)
		map = vi_insertion_keymap;
	cur = rl_function_of_keyseq(s.key, map, NULL);
	if (l->slot_user >> i & 1)
		return ;
	l->slot_bits &= ~(1u << i);
	if (cur != rl_slot_mark)
		l->slot_user |= 1u << i;
	else if (l->feat_want[s.feat])
		l->slot_bits |= 1u << i;
	if (cur == rl_slot_mark && l->feat_want[s.feat])
		rl_bind_keyseq_in_map(s.key, repl, map);
	else if (cur == rl_slot_mark)
		rl_bind_keyseq_in_map(s.key, stock, map);
}
