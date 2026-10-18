/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_slot_tab.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* The table of slots (rl_slot.c holds the logic that binds them). */

/* The whole table, by index: 0-7 are the history keys (Up / Down, in the
   CSI and SS3 encodings, emacs then vi), 8-12 the emacs accept keys and
   13-16 the vi ones (which have no ^F); 17-24 are the word and cycle keys
   (rl_slot_extra). `vi` picks vi_insertion_keymap over the emacs one. */
t_slot	rl_slot_at(int i)
{
	static const char	*keys[] = {"\033[A", "\033[B", "\033OA", "\033OB",
		"\033[C", "\033OC", "\006", "\033[F", "\033OF"};
	t_slot				s;
	int					k;
	int					j;

	if (i >= 17)
		return (rl_slot_extra(i - 17));
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

/* Slots 17-24: Alt-F, Ctrl-Right (two encodings) take a word of the
   ghost, Alt-N / Alt-P cycle it -- 0-4 in the emacs keymap, 5-7 (Alt-F,
   Alt-N, Alt-P) in vi insert mode. `\033O5C` is left alone: readline has
   nothing on it, so there is no stock function to give back. */
t_slot	rl_slot_extra(int j)
{
	static const char	*keys[] = {"\033f", "\033[1;5C", "\033[5C",
		"\033n", "\033p"};
	t_slot				s;
	int					k;

	k = j;
	if (j == 5)
		k = 0;
	if (j > 5)
		k = j - 3;
	s.feat = FEAT_ACCEPT;
	s.vi = j > 4;
	s.key = keys[k];
	s.fn = SLOT_WORD + (k > 2) + (k > 3);
	return (s);
}

/* The stock function of a slot, and what replaces it while its feature
   is on. In vi insert mode the Alt keys have no stock function (ESC
   leaves insert mode), so there the slot is unbound while off. */
void	rl_slot_funcs(const t_slot *s, rl_command_func_t **stock,
	rl_command_func_t **repl)
{
	static rl_command_func_t *const	stk[] = {rl_get_previous_history,
		rl_get_next_history, rl_forward_char, rl_end_of_line,
		rl_forward_word, rl_noninc_forward_search,
		rl_noninc_reverse_search};
	static rl_command_func_t *const	rep[] = {rl_history_search_backward,
		rl_history_search_forward, rl_suggest_accept, rl_suggest_accept,
		rl_suggest_word, rl_suggest_older, rl_suggest_newer};

	*stock = stk[s->fn];
	*repl = rep[s->fn];
	if (s->vi && s->fn >= SLOT_WORD)
		*stock = NULL;
}
