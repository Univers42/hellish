/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_multi_line3.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* The dialect switch is a lexing hazard -- issue #112.
**
** `emulate zsh` and `set -o zsh` change how EVERY LATER LINE lexes: a bare
** `}` closes a group, `} always {` is a keyword, `for a b (x y)` parses.
** They therefore belong with alias, source and shopt in input_hazard_at,
** which is what makes a batch of lines stop AT the switch so the rest of
** the file is tokenised after it ran. Without this, a config that opened
** with `emulate zsh` and continued with the zsh idiom every prompt tutorial
** teaches -- `precmd() { vcs_info }` -- was lexed whole before the first
** line executed, and died as "syntax error: unexpected end of file" with
** the switch sitting right there on line 1.
**
** Word-bounded on the left so `preemulate` or `unset -o` cannot trip it, but
** deliberately not on the command position: a hazard is a conservative
** clip, and a false positive only costs one shorter batch. `set +o` rides
** along so `set +o zsh` leaves the dialect on the same line it arrived.
*/
bool	dialect_hazard_at(const char *s, size_t i, size_t n)
{
	if (i > 0 && ft_strchr(" \t\n;&|(){}", s[i - 1]) == NULL)
		return (false);
	if (s[i] == 'e' && n - i >= 7 && ft_strncmp(s + i, "emulate", 7) == 0)
		return (i + 7 == n || ft_strchr(" \t\n;&|", s[i + 7]) != NULL);
	if (s[i] == 's' && n - i >= 6 && (ft_strncmp(s + i, "set -o", 6) == 0
			|| ft_strncmp(s + i, "set +o", 6) == 0))
		return (true);
	return (false);
}
