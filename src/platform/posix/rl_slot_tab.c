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

/* Readline's own functions, by the name readline knows them under:
   0-6 the stock function of each SLOT_*, 7-8 the history-search pair
   that replaces Up / Down.
     Looked up by name, never by address. In a non-PIE executable (clang
   before 15 builds those by default, 42's machines included)
   `&rl_forward_char` is the executable's PLT stub, while a libreadline
   linked -Bsymbolic-functions (Debian, Ubuntu) keeps its internal address
   in the keymaps. The two never compare equal, so every slot read as the
   user's own binding and no feature key was installed: Up stayed plain
   history and Right / Alt-F / Alt-N never touched the ghost. A search
   bound by stub address would also miss readline's rl_last_func check,
   so a second Up would restart the search instead of walking back. */
static rl_command_func_t	*rl_slot_readline(int i)
{
	static const char *const	names[] = {"previous-history",
		"next-history", "forward-char", "end-of-line", "forward-word",
		"non-incremental-forward-search-history",
		"non-incremental-reverse-search-history",
		"history-search-backward", "history-search-forward"};

	return (rl_named_function(names[i]));
}

/* The stock function of a slot, and what replaces it while its feature
   is on. In vi insert mode the Alt keys have no stock function (ESC
   leaves insert mode), so there the slot is unbound while off. */
void	rl_slot_funcs(const t_slot *s, rl_command_func_t **stock,
	rl_command_func_t **repl)
{
	static rl_command_func_t *const	rep[] = {NULL, NULL, rl_suggest_accept,
		rl_suggest_accept, rl_suggest_word, rl_suggest_older,
		rl_suggest_newer};

	*stock = rl_slot_readline(s->fn);
	*repl = rep[s->fn];
	if (s->fn <= SLOT_NEXT)
		*repl = rl_slot_readline(SLOT_NEWER + 1 + s->fn);
	if (s->vi && s->fn >= SLOT_WORD)
		*stock = NULL;
}
