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

/* The whole table, by index: 0-7 are the history keys (Up / Down, in the
   CSI and SS3 encodings, emacs then vi), 8-12 the emacs accept keys and
   13-16 the vi ones (which have no ^F). `vi` picks vi_insertion_keymap
   over the emacs one. */
t_slot	rl_slot_at(int i)
{
	static const char	*keys[] = {"\033[A", "\033[B", "\033OA", "\033OB",
		"\033[C", "\033OC", "\006", "\033[F", "\033OF"};
	t_slot				s;
	int					k;
	int					j;

	s.feat = FEAT_HIST;
	s.vi = i / 4;
	s.key = keys[i % 4];
	s.fn = SLOT_PREV + i % 2;
	if (i < 8)
		return (s);
	j = i - 8;
	k = 4 + j;
	s.feat = FEAT_ACCEPT;
	s.vi = j > 4;
	if (s.vi)
		k = j - 1 + (j > 6);
	s.key = keys[k];
	s.fn = SLOT_FWD + (k > 6);
	return (s);
}

/* The stock function of a slot, and what replaces it while its feature
   is on. */
static void	rl_slot_funcs(const t_slot *s, rl_command_func_t **stock,
	rl_command_func_t **repl)
{
	*stock = rl_get_previous_history;
	*repl = rl_history_search_backward;
	if (s->fn == SLOT_NEXT)
	{
		*stock = rl_get_next_history;
		*repl = rl_history_search_forward;
	}
	if (s->fn == SLOT_FWD || s->fn == SLOT_END)
		*repl = rl_suggest_accept;
	if (s->fn == SLOT_FWD)
		*stock = rl_forward_char;
	if (s->fn == SLOT_END)
		*stock = rl_end_of_line;
}

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
